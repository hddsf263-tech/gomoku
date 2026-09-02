# 五子棋 AI 人机对弈系统重建 - Phase 1 架构设计

**日期**: 2026-09-01
**阶段**: Phase 1 - AI 架构设计（只做设计，不写 AI 算法）
**基线**: `D:\gomoku` 基础双人对弈版（Build PASS / Runtime PASS / HVH PASS）
**原则**: 不接回旧 AI；C 盘仅只读历史参考；AI 核心不依赖 UI。

---

## 1. 当前基线分析

### 1.1 现有基础类型与接口

| 文件 | 现有能力 | 与 AI 重建的关系 |
| --- | --- | --- |
| `include/ChessPiece.h` | `ChessPiece`、`GameState`、`PlayerType` 枚举 | `PlayerType::Human/AI` 已存在，可直接复用 |
| `include/Board.h` + `src/Board.cpp` | 15x15 棋盘、落子、悔棋、五连判断、拷贝语义 | 可拷贝，适合作为 AI 搜索的快照载体 |
| `include/Game.h` + `src/Game.cpp` | 回合、落子、胜负、悔棋、回调 | 缺少玩家类型、AI 回合判断、和棋检测 |
| `src/MainWindow.h/.cpp` | 纯双人 UI 流程 | 后续加入 AI 调度，但 AI 不直接操作 UI |
| `src/GameModeDialog.h/.cpp` | 仅“双人对战”确认框 | 后续扩展为 HvH / HvA 选择 |
| `src/BoardWidget.h/.cpp` | 棋盘绘制、鼠标坐标转换 | 无需改动核心逻辑 |

### 1.2 当前缺口

1. `Game` 没有 `blackPlayerType` / `whitePlayerType`，无法判断 AI 回合。
2. `Game::checkGameEnd()` 未处理棋盘满和棋。
3. 项目中没有 AI 核心、AI 配置、取消机制。
4. 没有异步调度层，AI 若直接运行会阻塞 UI。
5. `GameModeDialog` 只有双人模式，没有人机模式入口。

### 1.3 历史参考范围

旧 AI（`LocalAIPlayer` / `SearchEngine` 等）已归档在：

```text
C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku_ai_archive_20260901
```

本次只参考其中的算法思路和历史 bug 教训（尤其 `isGameOver()`），不复制旧实现。

---

## 2. 目标架构

```text
UI Layer (Qt)
MainWindow / BoardWidget / GameModeDialog
        ↓ 人类点击
Game Controller Layer
Game（状态/规则/玩家类型）+ AIMoveScheduler（异步调度）
        ↓ 只读快照
Player Interface
IPlayer::getMove(GameStateSnapshot) -> Move
        ↓
AI Core（纯 C++17，不依赖 Qt）
AIPlayer -> SearchEngine -> Evaluation / MoveGenerator / ZobristHash / TranspositionTable
```

核心约束：

- AI 核心不得 include Qt 头文件。
- AI 只能接收 `GameStateSnapshot` 副本，不能拿到 `Game*` 或 UI 对象。
- 结果必须回到主线程，校验合法性后再落子。

---

## 3. 建议文件结构

```text
D:\gomoku
├── include
│   ├── Board.h
│   ├── ChessPiece.h
│   ├── Game.h
│   └── ai
│       ├── AIConfig.h
│       ├── AIPlayer.h
│       ├── Evaluation.h
│       ├── GameStateSnapshot.h
│       ├── HumanPlayer.h
│       ├── IPlayer.h
│       ├── MoveGenerator.h
│       ├── SearchEngine.h
│       ├── SearchResult.h
│       ├── TranspositionTable.h
│       └── ZobristHash.h
├── src
│   ├── Board.cpp
│   ├── Game.cpp
│   ├── MainWindow.cpp
│   ├── GameModeDialog.cpp
│   ├── AIMoveScheduler.h/.cpp       （Qt 异步调度，允许依赖 Qt）
│   └── ai
│       ├── AIPlayer.cpp
│       ├── Evaluation.cpp
│       ├── HumanPlayer.cpp
│       ├── MoveGenerator.cpp
│       ├── SearchEngine.cpp
│       ├── TranspositionTable.cpp
│       └── ZobristHash.cpp
└── tests
    └── ai_core_tests.cpp             （无 Qt 依赖的独立测试）
```

说明：`AIMoveScheduler` 属于 UI/调度边界，可以依赖 Qt；`src/ai` 与 `include/ai` 内不得依赖 Qt。

---

## 4. 接口设计

### 4.1 数据快照

```cpp
namespace Gomoku {

struct Move {
    int row = -1;
    int col = -1;
    bool valid = false;
};

struct GameStateSnapshot {
    Board board;              // 值拷贝，与 Game 脱钩
    ChessPiece currentPlayer = ChessPiece::Black;
    GameState status = GameState::InProgress;
    int moveCount = 0;
};

}
```

### 4.2 玩家接口

```cpp
namespace Gomoku {

class IPlayer {
public:
    virtual ~IPlayer() = default;
    virtual Move getMove(const GameStateSnapshot& state) = 0;
    virtual void cancel() {}
};

}
```

- `HumanPlayer`：实现 IPlayer，用于对称设计；实际 UI 流程中人类点击直接调用 `Game::makeMove()`，不在 AI 线程运行。
- `AIPlayer`：实现 IPlayer，内部持有 `SearchEngine` 与 `AIConfig`，`getMove()` 在线程池中被调用。

### 4.3 AI 配置

```cpp
struct AIConfig {
    int maxDepth = 4;              // 深度限制
    int timeLimitMs = 2000;        // 时间限制
    bool useTT = true;             // 置换表
    bool useIterativeDeepening = true;
    bool useMoveOrdering = true;
    size_t ttSizeMB = 64;
    uint64_t maxNodes = 1000000;   // 节点保护
};
```

### 4.4 搜索结果

```cpp
struct SearchResult {
    Move move;
    bool cancelled = false;
    bool timedOut = false;
    bool nodeLimitReached = false;
    uint64_t nodesVisited = 0;
    double timeMs = 0.0;
    int completedDepth = 0;
};
```

---

## 5. 搜索安全机制

每次搜索必须具备：

| 机制 | 设计 |
| --- | --- |
| 时间限制 | `timeLimitMs`，每个节点或每 N 个节点检查一次 |
| 深度限制 | `maxDepth`，默认 4 |
| 节点保护 | `maxNodes`，达到后立即返回当前最佳结果 |
| 超时返回 | 停止新分支，返回上一完整深度或当前最佳着法 |
| 取消机制 | `std::atomic<bool> cancelled`，`cancel()` 置位，搜索尽快返回 |
| 终局停止 | 五连或棋盘满后直接返回，不再搜索 |

搜索流程：

```text
校验快照
→ 终局检查（胜负/和棋）-> 直接返回
→ Iterative Deepening：depth = 1..maxDepth
→ 每层检查 timeout / maxNodes / cancel
→ 保留最近一个完整深度的最佳着法
→ 返回 SearchResult
```

---

## 6. AI Core 组件职责

| 组件 | 职责 |
| --- | --- |
| `SearchEngine` | Minimax + Alpha-Beta + Iterative Deepening + Move Ordering |
| `MoveGenerator` | 生成候选点，优先棋子附近空位 |
| `Evaluation` | 静态评估：五连、活四、冲四、活三等棋型 + 位置分 |
| `ZobristHash` | 64 位局面哈希，增量更新 |
| `TranspositionTable` | 局面缓存，深度优先替换 |
| `AIPlayer` | 组装搜索并返回 `Move`，支持 `cancel()` |

终局判断设计：

- 胜利：用 `Board::checkFiveInRow(lastMove.row, lastMove.col, piece)`。
- 和棋：`moveCount >= BOARD_SIZE * BOARD_SIZE`。
- 已结束局面：搜索直接返回，不继续展开。

---

## 7. 异步调度设计

### 7.1 调度边界

- `Game` 与 AI 核心不负责启动线程。
- `AIMoveScheduler` 负责：
  - 从 `Game` 取 `GameStateSnapshot` 副本；
  - 在线程池调用 `AIPlayer::getMove()`；
  - 通过 `QFutureWatcher` 把结果送回主线程；
  - 校验结果位置合法且游戏仍在进行；
  - 调用 `Game::makeMove()`；
  - 支持 `cancel()`（重开/退出/切换模式时调用）。

### 7.2 防重入

- AI 思考期间禁止棋盘点击落子。
- 同一时刻只允许一个 AI 搜索。
- 新游戏 / 退出 / 切换模式时先 `cancel()` 再等旧任务结束。

### 7.3 调用链

```text
人类落子
→ Game::makeMove()
→ 若 isAITurn() && InProgress
→ AIMoveScheduler::schedule(snapshot)
→ 后台 SearchEngine 计算
→ QFutureWatcher::finished
→ 主线程校验并 Game::makeMove(AI Move)
→ 更新 UI
```

---

## 8. Game / UI 扩展计划（后续阶段）

### 8.1 Game

- 恢复 `blackPlayerType` / `whitePlayerType`（干净重写，不复制旧代码）。
- 增加 `setPlayerType()`、`isAITurn()`。
- 增加 `getGameStateSnapshot()`。
- `checkGameEnd()` 增加和棋检测。

### 8.2 GameModeDialog

- 增加 Human vs Human / Human vs AI。
- 人机模式增加人类执黑/执白、AI 难度。
- 默认仍为双人对战，不破坏现有双人流程。

### 8.3 MainWindow

- 接入 `AIMoveScheduler`。
- AI 思考期间显示状态并禁用输入。
- 重开/退出时取消搜索。

---

## 9. 独立测试计划

在接入 UI 前完成 `ai_core_tests`：

| 测试 | 内容 | 通过标准 |
| --- | --- | --- |
| Test 1 | 合法落子 | 1000 个随机局面均返回棋盘内空位 |
| Test 2 | 立即获胜 | 四连 + 空位，AI 返回获胜点 |
| Test 3 | 立即防守 | 对手四连 + 空位，AI 阻止 |
| Test 4 | 终局 | 棋盘满或已结束，搜索立即返回 |
| Test 5 | 时间限制 | `timeLimitMs=1000`，返回时间有界 |
| Test 6 | 取消搜索 | 搜索中 `cancel()`，可及时返回 |
| Test 7 | 重复调用 | 连续 100 次不崩溃 |

测试程序不依赖 Qt，作为独立 CMake 目标 `ai_core_tests`。

---

## 10. 检查点 1 验收项

- [x] 分析当前 Board / Game / MainWindow / GameModeDialog
- [x] AI 与游戏逻辑接口已定义
- [x] 线程方案已定义（QtConcurrent + 快照 + 主线程回投）
- [x] 文件结构已定义
- [x] 状态管理与取消机制已定义
- [ ] AI Core 实现（下一阶段）
- [ ] 独立测试（下一阶段）

---

## 11. 本阶段执行记录

- 读取任务说明文件
- 核对 `D:\gomoku` 当前基线源码
- 分析旧 AI 归档中的历史教训（`isGameOver()` bug、两代 AI 并存问题）
- 生成本设计文档
- 本阶段未修改任何源代码或工程配置
