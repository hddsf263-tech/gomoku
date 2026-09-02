# AI 集成审计报告

**日期**: 2026-09-01  
**审计范围**: 完整的人机对弈调用链

---

## 一、当前架构状态

### 1.1 已存在的 AI 模块（✓ 正常）

| 模块 | 文件 | 状态 |
|------|------|------|
| AIPlayer | src/AIPlayer.cpp, include/AIPlayer.h | ✓ 存在且编译 |
| SearchEngine | src/SearchEngine.cpp, include/SearchEngine.h | ✓ 存在且编译 |
| ZobristHash | src/ZobristHash.cpp, include/ZobristHash.h | ✓ 存在且编译 |
| TranspositionTable | src/TranspositionTable.cpp, include/TranspositionTable.h | ✓ 存在且编译 |
| Game AI 集成 | src/Game.cpp, include/Game.h | ✓ 存在 |

### 1.2 调用链分析

#### 当前实际调用链：

```
MainWindow::onPositionClicked()
    ↓
Game::makeMove(row, col)        [人类落子]
    ↓
board.placePiece()
    ↓
checkGameEnd()
    ↓
switchPlayer()
    ↓
[返回 MainWindow]
    ↓
检查 isAITurn() → 如果是，调用 makeAIMove()
    ↓
AIPlayer::getBestMove()
    ↓
SearchEngine::findBestMove()
    ↓
Negamax + TT + Zobrist + ID
    ↓
返回 (row, col)
    ↓
Game::makeMove()  [AI 落子]
    ↓
刷新 UI
```

#### 发现的问题：

**核心问题：没有游戏模式选择入口**

当前 MainWindow 中：
- 没有"游戏模式"选择（双人对战 vs 人机对战）
- 没有"玩家颜色"选择（执黑 vs 执白）
- 没有"AI 难度"选择
- `Game` 的 `blackPlayerType` 和 `whitePlayerType` 默认为 `Human`
- 用户无法调用 `Game::setPlayerType()` 来设置 AI 对手

**结果**：即使 AI 代码完整，用户也无法进入人机对战模式。

---

## 二、详细问题清单

### 2.1 UI 层缺失

| 功能 | 状态 | 说明 |
|------|------|------|
| 游戏模式选择 | ❌ 缺失 | 无 HumanVsHuman / HumanVsAI 选择 |
| 玩家颜色选择 | ❌ 缺失 | 无法选择执黑或执白 |
| AI 难度选择 | ❌ 缺失 | 无法调整 AI 深度/时间 |
| "开始新游戏"配置 | ❌ 不完整 | 直接开始，无配置对话框 |

### 2.2 Game 层问题

| 问题 | 状态 | 影响 |
|------|------|------|
| setPlayerType 存在但未被调用 | ⚠️ 未使用 | 无法设置 AI 对手 |
| 默认双人类 | ⚠️ 限制 | 只能双人对战 |

### 2.3 线程/阻塞问题

| 问题 | 状态 | 风险 |
|------|------|------|
| AI 搜索在主线程 | ⚠️ 阻塞 | 困难模式可能卡死 UI 5 秒 |
| 无异步搜索 | ⚠️ 缺失 | 用户体验差 |
| 无"思考中"状态 | ⚠️ 缺失 | 用户不知道 AI 在行动 |

### 2.4 日志/诊断

| 问题 | 状态 |
|------|------|
| 无 AI 搜索日志 | ❌ |
| 无着法记录 | ❌ |
| 无错误报告 | ❌ |

---

## 三、必须修复的问题

### 优先级 P0（必须）

1. **添加游戏模式选择 UI**
   - 双人对战 / 人机对战 单选
   - 人机模式下：玩家颜色选择（黑/白）
   - AI 难度选择（简单/标准/困难）

2. **在开始新游戏时应用模式设置**
   - 调用 `Game::setPlayerType()` 设置 AI
   - 如果玩家选白，AI 自动先手

3. **添加 AI 回合状态指示**
   - "AI 思考中..."提示
   - 禁用棋盘输入

### 优先级 P1（重要）

4. **异步 AI 搜索**
   - 使用 QThread 或 QtConcurrent
   - 防止 UI 阻塞

5. **添加诊断日志**
   - AI 搜索开始/结束
   - 最佳着法
   - 搜索统计

### 优先级 P2（建议）

6. **改进悔棋逻辑**
   - 人机模式下双步悔棋

7. **重新开始时取消 AI 搜索**
   - 防止旧搜索污染新棋局

---

## 四、调用链验证

### 问题 1: AIPlayer 是否真的被创建？
**答案**: ✓ 是
```cpp
// Game.cpp
Game::Game() : aiPlayer(new AIPlayer()) {}
```

### 问题 2: AIPlayer 是否真的被调用？
**答案**: ✓ 是（但仅在 isAITurn() 为真时）
```cpp
// Game.cpp
void Game::makeAIMove() {
    auto [row, col] = aiPlayer->getBestMove(board, currentPlayer);
}
```

### 问题 3: 玩家落子后是否判断 AI 回合？
**答案**: ✓ 是（MainWindow 已修复）
```cpp
// MainWindow.cpp
if (game.isAITurn()) {
    game.makeAIMove();
}
```

### 问题 4: AI 是否会自动调用搜索？
**答案**: ✓ 是
```cpp
// AIPlayer.cpp
engine.findBestMove(board, player, config.maxDepth, ...)
```

### 问题 5: AI 搜索完成后是否向 Board 写入棋子？
**答案**: ✓ 是
```cpp
// Game.cpp
makeMove(row, col);  // 会调用 board.placePiece()
```

### 问题 6: AI 落子后是否触发胜负判断？
**答案**: ✓ 是
```cpp
// Game.cpp
checkGameEnd(row, col);
```

### 问题 7: AI 落子后 BoardWidget 是否刷新？
**答案**: ✓ 是（通过回调）
```cpp
// MainWindow.cpp
game.onMoveMade([this](int r, int c) {
    boardWidget->updateBoard();
});
```

### 问题 8: UI 是否存在"人机对战"模式？
**答案**: ❌ **否 - 这是核心问题**

### 问题 9: UI 是否存在选择玩家执黑/执白的功能？
**答案**: ❌ 否

### 问题 10: UI 是否存在 AI 难度选择？
**答案**: ❌ 否

### 问题 11: 是否存在"AI 回合"的状态控制？
**答案**: ⚠️ 部分 - 有 isAITurn() 但无 UI 状态指示

### 问题 12: 是否因为之前的修改绕开了 AI 调用？
**答案**: ⚠️ 是 - 因为没有设置 PlayerType 为 AI

---

## 五、根本原因

**人机对弈功能"消失"的真正原因**：

1. **CMakeLists.txt 缺少 LocalAIPlayer.cpp**（已修复）
2. **MainWindow 没有在游戏开始时调用 setPlayerType()** ← 当前主要问题
3. **没有 UI 让用户选择游戏模式** ← 需要新增

---

## 六、修复计划

### 步骤 1: 添加游戏模式对话框
- 新建 GameModeDialog 类
- 提供模式/颜色/难度选择

### 步骤 2: 修改 MainWindow
- 点击"新游戏"时弹出对话框
- 根据选择调用 Game::setPlayerType()
- 处理玩家执白时 AI 先手

### 步骤 3: 添加 AI 思考状态
- 禁用棋盘输入
- 显示"AI 思考中..."

### 步骤 4: 异步搜索（可选但推荐）
- 使用 QtConcurrent::run()
- 信号槽通知结果

### 步骤 5: 添加日志
- qDebug() 输出关键事件

---

**审计完成时间**: 2026-09-01  
**下一步**: 开始实施修复
