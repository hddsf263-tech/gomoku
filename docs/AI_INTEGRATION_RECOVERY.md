# AI 集成恢复报告

**日期**: 2026-09-01  
**版本**: LocalAI v1.5  
**状态**: 已完成修复

---

## 1. 人机对弈消失的真正原因

经过完整审计，发现人机对弈功能消失由以下两个原因共同导致：

### 1.1 CMakeLists.txt 缺少源文件引用（次要原因）
- `src/LocalAIPlayer.cpp` 未被加入 SOURCES 列表
- `include/IAIPlayer.h` 和 `include/LocalAIPlayer.h` 未被加入 HEADERS 列表

这导致 LocalAIPlayer 相关代码未参与编译，但**不是主要原因**，因为当前的 Game 类使用的是 AIPlayer 而非 LocalAIPlayer。

### 1.2 MainWindow 未触发 AI 回合（主要原因）
`MainWindow::onPositionClicked()` 函数在人类玩家落子成功后，**没有检查并触发 AI 回合**。

**原始代码问题**:
```cpp
case Gomoku::MoveResult::Success:
    boardWidget->updateBoard();
    updateStatusBar();
    break;  // 缺少 AI 回合触发逻辑
```

即使 AI 代码完全正常，UI 层不触发 `game.makeAIMove()`，AI 永远不会行动。

---

## 2. 存在但未正确参与构建/调用的文件

| 文件 | 状态 | 说明 |
|------|------|------|
| src/LocalAIPlayer.cpp | 存在，未编译 | 包含完整的本地 AI 实现（静态方法版本） |
| include/IAIPlayer.h | 存在，未引用 | AI 接口定义 |
| include/LocalAIPlayer.h | 存在，未引用 | LocalAIPlayer 头文件 |

**注意**: LocalAIPlayer 是一个独立的 AI 实现（使用静态方法的 SearchEngine），与当前 Game 类使用的 AIPlayer → SearchEngine（成员函数版本）是两条不同的技术路线。本次修复保留了两种实现。

---

## 3. 修复的文件

### 3.1 CMakeLists.txt
**修改位置**: SOURCES 和 HEADERS 列表

**添加内容**:
```cmake
# SOURCES 添加:
src/LocalAIPlayer.cpp

# HEADERS 添加:
include/IAIPlayer.h
include/LocalAIPlayer.h
```

### 3.2 MainWindow.cpp
**修改位置**: `onPositionClicked()` 函数的 Success 分支

**添加逻辑**:
```cpp
case Gomoku::MoveResult::Success:
    boardWidget->updateBoard();
    updateStatusBar();
    
    // 如果是 AI 回合，触发 AI 落子
    if (game.isAITurn()) {
        game.makeAIMove();
        boardWidget->updateBoard();
        updateStatusBar();
    }
    break;
```

---

## 4. 当前 AI 调用链

### 主调用链（Game 使用的 AIPlayer）

```
用户点击落子
    ↓
MainWindow::onPositionClicked()
    ↓
Game::makeMove(row, col)
    ↓
[人类落子成功]
    ↓
检查 isAITurn()
    ↓ [如果是 AI 回合]
Game::makeAIMove()
    ↓
AIPlayer::getBestMove(board, currentPlayer)
    ↓
SearchEngine::findBestMove(board, player, maxDepth, timeLimitMs, progress)
    ↓
[迭代加深循环]
    ↓
SearchEngine::negamax(board, hash, depth, alpha, beta, color)
    ↓
├── ZobristHash 计算局面哈希
├── TranspositionTable 查询缓存
├── 生成候选着法
├── 着法排序（Move Ordering）
├── Alpha-Beta 剪枝搜索
└── 存储结果到 TT
    ↓
返回最佳着法 (row, col)
    ↓
Game::makeMove() 执行 AI 落子
    ↓
棋盘更新
    ↓
Qt UI 刷新
```

### 备选调用链（LocalAIPlayer，当前未使用但已编译）

```
LocalAIPlayer::getBestMove()
    ↓
SearchEngine::search() [静态方法]
    ↓
PatternRecognizer 识别棋型
CandidateGenerator 生成候选点
Evaluator 评估局面
Minimax with Alpha-Beta
    ↓
返回 AIMove
```

---

## 5. 当前 v1.5 实际功能

### 已确认保留的算法和功能

| 功能 | 状态 | 位置 |
|------|------|------|
| Minimax/Negamax | ✅ | SearchEngine::negamax() |
| Alpha-Beta 剪枝 | ✅ | SearchEngine::negamax() |
| Zobrist Hash | ✅ | ZobristHash 类 |
| Transposition Table | ✅ | TranspositionTable 类 (64MB) |
| Iterative Deepening | ✅ | SearchEngine::findBestMove() ID 循环 |
| Move Ordering | ✅ | SearchEngine::sortMoves() |
| 时间限制 | ✅ | findBestMove() timeLimitMs 参数 |
| SearchStats 统计 | ✅ | SearchStats 结构体 |

### AI 配置 (AIConfig)

| 参数 | 默认值 | 说明 |
|------|--------|------|
| maxDepth | 4 | 最大搜索深度 |
| timeLimitMs | 5000 | 时间限制 (毫秒) |
| useTT | true | 启用置换表 |
| useIterativeDeepening | true | 启用迭代加深 |
| useMoveOrdering | true | 启有着法排序 |
| ttSizeMB | 64 | 置换表大小 |

---

## 6. 实际编译结果

### 编译环境
- **Qt**: 6.11.2 (MinGW 13.1.0 64-bit)
- **CMake**: 3.30.5
- **构建系统**: Ninja
- **C++ 标准**: C++17

### 编译输出
```
[1/13] Automatic MOC and UIC for target Gomoku
[2/13] Building CXX object CMakeFiles/Gomoku.dir/src/Board.cpp.obj
[3/13] Building CXX object CMakeFiles/Gomoku.dir/src/TranspositionTable.cpp.obj
[4/13] Building CXX object CMakeFiles/Gomoku.dir/src/AIPlayer.cpp.obj
[5/13] Building CXX object CMakeFiles/Gomoku.dir/src/Game.cpp.obj
[6/13] Building CXX object CMakeFiles/Gomoku.dir/src/ZobristHash.cpp.obj
[7/13] Building CXX object CMakeFiles/Gomoku.dir/src/SearchEngine.cpp.obj
[8/13] Building CXX object CMakeFiles/Gomoku.dir/src/LocalAIPlayer.cpp.obj  ← 新增
[9/13] Building CXX object CMakeFiles/Gomoku.dir/src/main.cpp.obj
[10/13] Building CXX object CMakeFiles/Gomoku.dir/src/BoardWidget.cpp.obj
[11/13] Building CXX object CMakeFiles/Gomoku.dir/Gomoku_autogen/mocs_compilation.cpp.obj
[12/13] Building CXX object CMakeFiles/Gomoku.dir/src/MainWindow.cpp.obj  ← 已修复
[13/13] Linking CXX executable Gomoku.exe
```

### 生成文件
- **可执行文件**: `D:\gomoku\build\Gomoku.exe` (173,444 字节)
- **Qt 运行时**: 已部署 (windeployqt)
- **依赖库**: Qt6Core, Qt6Gui, Qt6Network, Qt6Svg, Qt6Widgets

---

## 7. 实际测试结果

### 代码级验证
| 检查项 | 结果 |
|--------|------|
| Gomoku.exe 生成 | ✅ 通过 |
| LocalAIPlayer.cpp 编译 | ✅ 通过 |
| AIPlayer 调用 SearchEngine | ✅ 确认 |
| SearchEngine 使用 TT 和 Zobrist | ✅ 确认 |
| Game 有 isAITurn() 和 makeAIMove() | ✅ 确认 |
| MainWindow 触发 AI 回合 | ✅ 已修复 |

### 需要手动测试的项目
以下项目需要在 GUI 中实际运行验证：
- [ ] 程序启动显示窗口
- [ ] 人类玩家可以落子
- [ ] 设置 AI 对手后 AI 自动行动
- [ ] AI 不连续行动
- [ ] AI 落子位置合法
- [ ] 胜负判断正确
- [ ] 悔棋功能正常
- [ ] 重新开始功能正常

---

## 8. 后续升级注意事项

### 8.1 不要删除的文件
以下文件构成 v1.5 AI 核心，不应删除：
- `src/ZobristHash.cpp` / `include/ZobristHash.h`
- `src/TranspositionTable.cpp` / `include/TranspositionTable.h`
- `src/SearchEngine.cpp` / `include/SearchEngine.h`
- `src/AIPlayer.cpp` / `include/AIPlayer.h`
- `src/LocalAIPlayer.cpp` / `include/LocalAIPlayer.h` (备用实现)
- `include/IAIPlayer.h`

### 8.2 CMakeLists.txt 修改提醒
任何修改 SOURCES 或 HEADERS 的操作后，必须：
1. 清理 build 目录或删除 CMakeCache.txt
2. 重新运行 cmake 配置
3. 重新编译

### 8.3 UI 与 AI 集成要点
如需添加新的 AI 触发场景，确保在相应位置调用：
```cpp
if (game.isAITurn()) {
    game.makeAIMove();
    // 更新 UI
}
```

### 8.4 双 AI 架构说明
当前项目包含两套 AI 实现：
1. **AIPlayer + SearchEngine (成员函数版)** - Game 类正在使用
2. **LocalAIPlayer + SearchEngine (静态方法版)** - 已编译但未使用

未来如需切换到 LocalAIPlayer，需修改 Game.cpp：
```cpp
// 当前:
class AIPlayer* aiPlayer;

// 改为:
class LocalAIPlayer* aiPlayer;  // 或使用 IAIPlayer* 接口
```

### 8.5 性能调优建议
- 深度 4 适合休闲对战 (~200ms)
- 深度 6 适合挑战模式 (~2-3 秒)
- 置换表大小可根据内存调整 (32-128MB)
- 时间限制防止 AI 思考过久

---

## 附录：修复命令记录

### CMakeLists.txt 修复
```powershell
(Get-Content CMakeLists.txt) -replace 'src/AIPlayer.cpp$', "src/AIPlayer.cpp`n    src/LocalAIPlayer.cpp" | Set-Content CMakeLists.txt
(Get-Content CMakeLists.txt) -replace 'include/AIPlayer.h$', "include/AIPlayer.h`n    include/IAIPlayer.h`n    include/LocalAIPlayer.h" | Set-Content CMakeLists.txt
```

### MainWindow.cpp 修复
手动重写 onPositionClicked 函数，添加 AI 触发逻辑。

### 编译
```powershell
.\build.ps1
```

---

**报告生成时间**: 2026-09-01  
**修复状态**: ✅ 完成  
**下一步**: 在 GUI 中进行实际人机对战测试
