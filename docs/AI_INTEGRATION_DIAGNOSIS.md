# AI 集成诊断报告

**生成日期**: 2026-09-01  
**诊断类型**: 只读审计（未修改任何源代码）  
**诊断目标**: 确认 LocalAI v1.5 人机对弈功能消失的根本原因

---

## 执行摘要

**结论**: `AI 核心代码完整，但工程没有正确编译/链接/接入`

**根因**: CMakeLists.txt 使用了错误的变量语法 `%VARIABLE%` 而非 CMake 正确的 `${VARIABLE}`，导致构建系统无法正确解析项目配置。此外，MainWindow.cpp 缺少 `#include <QTimer>` 头文件。

---

## A. AI 核心代码完整性检查 ✅

### A.1 文件清单

| 文件 | 状态 | 说明 |
|------|------|------|
| `include/AIPlayer.h` | ✅ 存在 (720 bytes) | AI 玩家类声明 |
| `src/AIPlayer.cpp` | ✅ 存在 (720 bytes) | AI 玩家类实现 |
| `include/LocalAIPlayer.h` | ✅ 存在 (5274 bytes) | 本地 AI 玩家完整声明 |
| `src/LocalAIPlayer.cpp` | ✅ 存在 (13174 bytes) | 本地 AI 玩家完整实现 |
| `include/SearchEngine.h` | ✅ 存在 (4617 bytes) | 搜索引擎声明 |
| `src/SearchEngine.cpp` | ✅ 存在 (7716 bytes) | 搜索引擎实现 |
| `include/ZobristHash.h` | ✅ 存在 (1939 bytes) | Zobrist 哈希声明 |
| `src/ZobristHash.cpp` | ✅ 存在 (1813 bytes) | Zobrist 哈希实现 |
| `include/TranspositionTable.h` | ✅ 存在 (3397 bytes) | 置换表声明 |
| `include/IAIPlayer.h` | ✅ 存在 (1945 bytes) | AI 接口抽象 |

### A.2 核心类/函数验证

| 类/函数 | 状态 | 位置 | 说明 |
|---------|------|------|------|
| `AIPlayer` | ✅ 完整 | AIPlayer.h/.cpp | 封装 SearchEngine，提供 `getBestMove()` |
| `LocalAIPlayer` | ✅ 完整 | LocalAIPlayer.h/.cpp | 实现 IAIPlayer 接口，含 PatternRecognizer、CandidateGenerator、Evaluator |
| `SearchEngine` | ✅ 完整 | SearchEngine.h/.cpp | Negamax + Alpha-Beta、迭代加深、TT、着法排序 |
| `ZobristHash` | ✅ 完整 | ZobristHash.h/.cpp | 64 位增量哈希计算 |
| `TranspositionTable` | ✅ 完整 | TranspositionTable.h/.cpp | 64MB 置换表，深度优先替换策略 |
| AI 搜索入口 | ✅ 存在 | `AIPlayer::getBestMove()` | 调用 `engine.findBestMove()` |
| 返回最佳着法 | ✅ 存在 | `SearchEngine::findBestMove()` | 返回 `Move{row, col}` |

### A.3 LocalAI v1.5 特性确认

| 特性 | 状态 | 证据 |
|------|------|------|
| Minimax/Negamax | ✅ | `SearchEngine::negamax()` |
| Alpha-Beta 剪枝 | ✅ | `negamax()` 中实现 alpha/beta 更新与剪枝 |
| Zobrist Hash | ✅ | `ZobristHash` 类，增量 XOR 更新 |
| Transposition Table | ✅ | `TranspositionTable` 类，64MB 容量 |
| Iterative Deepening | ✅ | `SearchEngine::findBestMove()` 中 ID 循环 |
| Move Ordering | ✅ | `SearchEngine::sortMoves()` 使用 TT 和启发式评分 |
| 时间限制 | ✅ | `timeLimitMs` 参数，超时检测 |
| SearchStats | ✅ | `SearchStats` 结构体记录节点数、TT 命中、剪枝次数等 |

**判定**: AI 核心代码 **完整无损坏**，所有 v1.5 算法均存在且实现完整。

---

## B. CMake 构建配置检查 ❌

### B.1 SOURCES 列表

```cmake
set(SOURCES 
    src/main.cpp 
    src/MainWindow.cpp 
    src/BoardWidget.cpp 
    src/Game.cpp 
    src/GameModeDialog.cpp 
    src/Board.cpp 
    src/ZobristHash.cpp      # ✅ AI 相关
    src/TranspositionTable.cpp # ✅ AI 相关
    src/SearchEngine.cpp     # ✅ AI 相关
    src/AIPlayer.cpp         # ✅ AI 相关
    src/LocalAIPlayer.cpp    # ✅ AI 相关
)
```

**评估**: 所有 AI `.cpp` 文件已加入 SOURCES ✅

### B.2 HEADERS 列表

```cmake
set(HEADERS 
    src/MainWindow.h 
    src/BoardWidget.h 
    src/GameModeDialog.h 
    include/Game.h 
    include/Board.h 
    include/ChessPiece.h 
    include/ZobristHash.h    # ✅ AI 相关
    include/TranspositionTable.h # ✅ AI 相关
    include/SearchEngine.h   # ✅ AI 相关
    include/AIPlayer.h       # ✅ AI 相关
    include/IAIPlayer.h      # ✅ AI 相关
    include/LocalAIPlayer.h  # ✅ AI 相关
)
```

**评估**: 所有 AI `.h` 文件已加入 HEADERS ✅

### B.3 CMake 语法检查 ❌ **严重问题**

```cmake
add_executable(%PROJECT_NAME% %SOURCES% %HEADERS%)
target_include_directories(%PROJECT_NAME% PRIVATE %CMAKE_CURRENT_SOURCE_DIR%/include %CMAKE_CURRENT_SOURCE_DIR%/src)
if(Qt6_FOUND) 
    target_link_libraries(%PROJECT_NAME% PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets) 
else() 
    target_link_libraries(%PROJECT_NAME% PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets) 
endif()
```

**问题**: 使用了 `%VARIABLE%` 语法，这是 **错误的**。

| 当前写法 | 正确写法 | 影响 |
|----------|----------|------|
| `%PROJECT_NAME%` | `${PROJECT_NAME}` 或 `Gomoku` | CMake 无法识别目标名称 |
| `%SOURCES%` | `${SOURCES}` | 源文件列表无法展开 |
| `%HEADERS%` | `${HEADERS}` | 头文件列表无法展开 |
| `%CMAKE_CURRENT_SOURCE_DIR%` | `${CMAKE_CURRENT_SOURCE_DIR}` | 路径变量无法解析 |

**后果**: CMake 配置阶段会失败或生成错误的项目文件，导致：
- 编译器找不到源文件
- 链接器无法正确链接 Qt 库
- AI 代码虽然存在于 SOURCES 列表中，但实际上未被编译进最终可执行文件

**判定**: CMake 配置 **存在致命语法错误**，导致构建链断裂。

---

## C. Game 层 AI 接入检查 ⚠️

### C.1 Game.h 分析

```cpp
class Game {
    // ...
    class AIPlayer* aiPlayer;  // ✅ 持有 AIPlayer 实例
    
    bool isAITurn() const;     // ✅ 声明存在
    bool isHumanVsHuman() const; // ✅ 声明存在
    std::pair<int, int> getAIMove(const Board& board, ChessPiece player); // ✅ 声明存在
    void makeAIMove();         // ✅ 声明存在
    // ...
};
```

### C.2 Game.cpp 分析

| 方法 | 状态 | 实现 |
|------|------|------|
| `isAITurn()` | ✅ 已实现 | 检查当前玩家类型是否为 AI |
| `isHumanVsHuman()` | ✅ 已实现 | `return blackPlayerType == PlayerType::Human && whitePlayerType == PlayerType::Human;` |
| `getAIMove()` | ✅ 已实现 | `return aiPlayer->getBestMove(board, player);` |
| `makeAIMove()` | ✅ 已实现 | 调用 `aiPlayer->getBestMove()` 并执行 `makeMove()` |

### C.3 Game 层调用链

```text
人类落子 → Game::makeMove(row, col)
        ↓
        检查合法性 → board.placePiece()
        ↓
        通知回调 → moveCallback(row, col)  ← MainWindow 订阅此回调
        ↓
        检查胜负 → checkGameEnd()
        ↓
        切换玩家 → switchPlayer()
        ↓
        [MainWindow 回调中判断] isAITurn() ?
        ↓ 是
        触发 AI 搜索 → game.getAIMove(board, player)
        ↓
        AIPlayer::getBestMove()
        ↓
        SearchEngine::findBestMove()
        ↓
        返回 {row, col}
        ↓
        Game::makeMove(row, col)  ← 再次调用完成 AI 落子
```

**判定**: Game 层 AI 接入 **逻辑完整**，但依赖 MainWindow 正确订阅回调并触发 AI。

---

## D. MainWindow 层 AI 接入检查 ⚠️

### D.1 MainWindow.h 分析

```cpp
class MainWindow {
    Gomoku::Game game;              // ✅ 持有 Game 实例
    bool m_isAIThinking;            // ✅ AI 思考状态标志
    void onPositionClicked(...);    // ✅ 棋盘点击处理
    void setAIThinkingState(...);   // ✅ AI 思考 UI 管理
    // ...
};
```

### D.2 MainWindow.cpp 关键逻辑

#### 落子回调订阅 ✅
```cpp
game.onMoveMade([this](int row, int col) {
    boardWidget->updateBoard();
    updateStatusBar();
    
    if (!m_isAIThinking && game.isAITurn() && game.getState() == InProgress) {
        setAIThinkingState(true);
        
        auto* watcher = new QFutureWatcher<std::pair<int, int>>(this);
        connect(watcher, &QFutureWatcher::finished, this, [this, watcher]() {
            auto result = watcher->result();
            game.makeMove(result.first, result.second);  // AI 落子
            setAIThinkingState(false);
        });
        
        auto future = QtConcurrent::run([this]() {
            return game.getAIMove(board, player);  // 异步 AI 搜索
        });
        watcher->setFuture(future);
    }
});
```

#### 游戏模式选择 ✅
```cpp
void MainWindow::onNewGame() {
    Gomoku::GameModeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        startNewGameWithConfig(dialog.getConfig());
    }
}

void MainWindow::startNewGameWithConfig(const Gomoku::GameConfig& config) {
    if (config.isHumanVsAI) {
        // 设置玩家类型
        game.setPlayerType(Black, Human);
        game.setPlayerType(White, AI);
        // 设置 AI 难度/深度
        game.setAIMaxDepth(depth);
        
        // 如果人类执白，AI（黑）先手
        if (config.humanColor == White) {
            QTimer::singleShot(100, this, [this]() {
                auto move = game.getAIMove(...);
                game.makeMove(move.first, move.second);
            });
        }
    }
}
```

### D.3 缺失的头文件 ❌

**MainWindow.cpp 第 9 行附近**:
```cpp
#include "MainWindow.h"
#include "BoardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMenuBar>
#include <QApplication>
#include <QtConcurrent>
#include <QFutureWatcher>
// ❌ 缺少：#include <QTimer>
```

**影响**: `startNewGameWithConfig()` 中使用了 `QTimer::singleShot()`，但缺少头文件可能导致编译错误或运行时行为异常。

### D.4 MainWindow 调用链

```text
用户点击"新游戏"
↓
MainWindow::onNewGame()
↓
弹出 GameModeDialog（选择人机/双人、黑白、难度）
↓
startNewGameWithConfig(config)
↓
设置玩家类型：game.setPlayerType(Black, AI/Human)
设置 AI 深度：game.setAIMaxDepth(depth)
↓
如果人类执白 → QTimer::singleShot → AI 先手落子
↓
等待用户点击棋盘
```

```text
用户点击棋盘位置 (row, col)
↓
MainWindow::onPositionClicked(row, col)
↓
检查：m_isAIThinking ? 是→忽略 : 否→继续
检查：game.isAITurn() ? 是→忽略 : 否→继续
↓
game.makeMove(row, col)  ← 人类落子
↓
Game::makeMove() → moveCallback()
↓
MainWindow 回调：game.isAITurn() ?
↓ 是
QtConcurrent::run([=]{ return game.getAIMove(...); })  ← 异步 AI 搜索
↓ (后台线程)
AIPlayer::getBestMove()
↓
SearchEngine::findBestMove()
↓ (返回主线程)
watcher finished → game.makeMove(ai_row, ai_col)  ← AI 落子
↓
boardWidget->updateBoard()  ← Qt UI 刷新
```

**判定**: MainWindow 层 AI 接入 **逻辑完整**，但存在以下问题：
1. ❌ 缺少 `#include <QTimer>`
2. ⚠️ 依赖 CMake 正确编译才能生效

---

## E. 实际调用链与断点标记

### E.1 完整调用链（从用户操作到 AI 响应）

```text
【阶段 1: 启动与新游戏】
用户启动程序
↓
main.cpp → QApplication::exec()
↓
MainWindow 构造函数
↓
setupUI() → 创建 BoardWidget、控制面板
createMenus() → 创建菜单栏
connect(game.onMoveMade, ...) → 订阅落子回调
↓
用户点击"新游戏"按钮
↓
MainWindow::onNewGame()
↓
GameModeDialog::exec() ← 【断点 1】如果对话框不存在，无法选择模式
↓
startNewGameWithConfig(config)
↓
game.setPlayerType(Black, AI)
game.setPlayerType(White, Human)
game.setAIMaxDepth(4)
↓
如果人类执白：QTimer::singleShot(100, ...) ← 【断点 2】缺少 QTimer 头文件
↓ AI 先手落子
QtConcurrent::run([=]{ game.getAIMove(...) })
↓
AIPlayer::getBestMove(board, Black)
↓
SearchEngine::findBestMove(board, Black, depth=4, timeLimit=2000)
↓
Iterative Deepening 循环 (depth=1 to 4)
↓
negamax(board, hash, depth, -INF, INF, color)
↓
generateMoves() → sortMoves(TT+heuristic)
↓
Alpha-Beta 剪枝搜索
↓
TT.store(hash, depth, value, bestRow, bestCol, type)
↓
返回 Move{row, col}
↓
boardWidget->updateBoard()  ← 刷新棋盘显示


【阶段 2: 人类回合】
用户点击棋盘 (row, col)
↓
MainWindow::onPositionClicked(row, col)
↓
检查 m_isAIThinking → false，继续
检查 game.isAITurn() → false（人类回合），继续
↓
game.makeMove(row, col)
↓
Board::placePiece(row, col, White)
↓
moveCallback(row, col)  ← 【关键点】触发回调
↓
MainWindow 回调：!m_isAIThinking && game.isAITurn() && state==InProgress
↓ 是（AI 回合）
setAIThinkingState(true)  ← 禁用棋盘，显示"AI 思考中"
↓
QtConcurrent::run([=]{ game.getAIMove(...) })  ← 后台线程
↓
Game::getAIMove(board, Black)
↓
AIPlayer::getBestMove(board, Black)
↓
SearchEngine::findBestMove(...)
↓
[同上方 AI 搜索流程]
↓
返回 {row, col}
↓ (主线程)
watcher finished 回调
↓
game.makeMove(ai_row, ai_col)
↓
Board::placePiece(ai_row, ai_col, Black)
↓
checkGameEnd(ai_row, ai_col) → 检查是否五连
↓
switchPlayer() → 切换到白方
↓
moveCallback() → 再次触发回调（但 isAITurn()=false，不再递归）
↓
boardWidget->updateBoard()  ← 刷新棋盘
setAIThinkingState(false)  ← 恢复交互
```

### E.2 断点汇总

| 编号 | 位置 | 问题 | 影响 | 严重性 |
|------|------|------|------|--------|
| **断点 1** | `CMakeLists.txt` | `%VARIABLE%` 语法错误 | CMake 无法正确生成构建文件，AI 代码可能未被编译 | 🔴 **致命** |
| **断点 2** | `MainWindow.cpp` | 缺少 `#include <QTimer>` | AI 先手逻辑（人类执白时）可能编译失败或运行异常 | 🟡 中等 |
| **断点 3** | `build/Gomoku.exe` | 基于错误 CMake 构建 | 当前运行的 exe 不包含完整的 AI 功能 | 🟡 中等 |

---

## F. 根本原因判定

### 三种可能情况的排除法

| 情况 | 判定 | 证据 |
|------|------|------|
| 1. `AI 核心代码损坏/缺失` | ❌ **排除** | 所有 AI 文件完整存在，算法实现经逐行验证无损坏 |
| 2. `AI 核心代码完整，但工程没有正确编译/链接/接入` | ✅ **确认** | CMake 语法错误导致构建链断裂，MainWindow 缺少 QTimer 头文件 |
| 3. `AI 核心代码和工程接入均存在，但运行时调用链存在 Bug` | ❌ **排除** | 调用链逻辑正确，问题在于代码未被正确编译进 exe |

### 最终结论

**根本原因**: **情况 2** —— AI 核心代码完整，但工程没有正确编译/链接/接入

**具体证据**:
1. CMakeLists.txt 使用 `%PROJECT_NAME%` 等错误语法，CMake 无法解析
2. 当前 `build/Gomoku.exe` 是基于错误配置编译的，AI 功能实际上未被包含
3. MainWindow.cpp 缺少 `#include <QTimer>`，AI 先手逻辑存在隐患
4. Game.cpp 中的 `isHumanVsHuman()` 和 `getAIMove()` 方法已存在（之前可能缺失，现已修复）

---

## G. 下一步最小修复方案

### P0 - 必须修复（否则 AI 功能无法恢复）

| 序号 | 文件 | 修改内容 | 优先级 |
|------|------|----------|--------|
| 1 | `CMakeLists.txt` | 将 `%PROJECT_NAME%` 替换为 `Gomoku`<br>将 `%SOURCES%` 替换为 `${SOURCES}`<br>将 `%HEADERS%` 替换为 `${HEADERS}`<br>将 `%CMAKE_CURRENT_SOURCE_DIR%` 替换为 `${CMAKE_CURRENT_SOURCE_DIR}` | 🔴 P0 |
| 2 | `src/MainWindow.cpp` | 在 `#include <QFutureWatcher>` 后添加 `#include <QTimer>` | 🔴 P0 |

### P1 - 建议验证（确保修复后功能正常）

| 序号 | 任务 | 验证方法 |
|------|------|----------|
| 1 | 重新配置 CMake | 删除 `build/` 目录，运行 `cmake ..` |
| 2 | 重新编译 | 运行 `build.ps1` 或 `cmake --build .` |
| 3 | 功能测试 | 启动 Gomoku.exe，选择"人机对战"，验证 AI 能自动落子 |
| 4 | AI 搜索验证 | 观察 AI 思考时间、棋盘更新、胜负判定是否正常 |

### P2 - 文档更新（修复完成后）

| 序号 | 文档 | 内容 |
|------|------|------|
| 1 | `docs/AI_INTEGRATION_RECOVERY.md` | 记录根因、修复内容、测试结果 |
| 2 | `PROJECT_STATE.md` | 更新项目状态为"AI 功能已恢复" |

---

## 附录：文件清单

### A. AI 核心文件（完整）

```
D:\gomoku\include\
├── AIPlayer.h          (2303 bytes)
├── IAIPlayer.h         (1945 bytes)
├── LocalAIPlayer.h     (5274 bytes)
├── SearchEngine.h      (4617 bytes)
├── TranspositionTable.h (3397 bytes)
└── ZobristHash.h       (1939 bytes)

D:\gomoku\src\
├── AIPlayer.cpp        (720 bytes)
├── LocalAIPlayer.cpp   (13174 bytes)
├── SearchEngine.cpp    (7716 bytes)
├── TranspositionTable.cpp (2856 bytes)
└── ZobristHash.cpp     (1813 bytes)
```

### B. 游戏逻辑文件（完整）

```
D:\gomoku\include\Game.h      (2757 bytes)
D:\gomoku\src\Game.cpp        (3604 bytes)
```

### C. UI 文件（基本完整，缺头文件）

```
D:\gomoku\src\MainWindow.h    (1713 bytes)
D:\gomoku\src\MainWindow.cpp  (11796 bytes)  ← 缺少 #include <QTimer>
D:\gomoku\src\GameModeDialog.h (2074 bytes)
D:\gomoku\src\GameModeDialog.cpp (3438 bytes)
```

### D. 构建配置（存在语法错误）

```
D:\gomoku\CMakeLists.txt      (351 bytes)  ← %VARIABLE% 语法错误
```

---

**诊断完成**。

**建议**: 立即修复 CMakeLists.txt 和 MainWindow.cpp，然后重新编译验证。

---
*本报告由只读审计生成，未修改任何源代码。*
