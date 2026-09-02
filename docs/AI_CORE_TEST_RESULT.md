# AI Core 测试结果

**日期**: 2026-09-01
**阶段**: Phase 2A - Minimax + Alpha-Beta 基础搜索
**测试程序**: `D:\gomoku\build\ai_core_tests.exe`
**测试源码**: `D:\gomoku\tests\ai_core_tests.cpp`

## 构建结果

```text
Build:
PASS
```

## 测试结果

| 测试 | 结果 |
| --- | --- |
| Test 1: Legal Move x100 | PASS |
| Test 2: Immediate Win x4 directions | PASS |
| Test 3: Immediate Defense x4 directions | PASS |
| Test 4a: Terminal BlackWin no search | PASS |
| Test 4b: Terminal WhiteWin no search | PASS |
| Test 4c: Terminal external status no search | PASS |
| Test 5: Depth 1/2/3 legal return | PASS |
| Test 6: Repeated Calls x100 | PASS |

```text
Passed: 8
Failed: 0
```

## 统计

- 合法落子随机局面：100 次
- 立即获胜方向：4（横、竖、两条对角线）
- 立即防守方向：4（横、竖、两条对角线）
- 终局检测：3 种（黑胜、白胜、外部已结束状态）
- 搜索深度回归：1 / 2 / 3
- 重复调用：100 次
- 最大测试深度：3

## 尚未覆盖（后续内部阶段）

- Timeout（Phase 2B）
- Cancel（Phase 2B）
- Max Nodes（Phase 2B）
- Iterative Deepening（Phase 2B）
- ZobristHash（Phase 2C）
- TranspositionTable（Phase 2C）
- Move Ordering 回归（Phase 2C）

## 基线保护

- 主程序 `Gomoku` 重新编译通过。
- 双人对弈冒烟测试再次通过（启动、交替落子、黑方五连获胜、重新开始、重复落子拦截）。
- 未接入任何 UI / Human vs AI 代码。

## Phase 2B 结果

| 测试 | 结果 |
| --- | --- |
| Test 7: Timeout bounded return | PASS |
| Test 8: Cancel returns safely and timely | PASS |
| Test 9: Node limit returns legal move | PASS |
| Test 10: Iterative Deepening completes depth 3 | PASS |

```text
Passed: 12
Failed: 0
```

## Phase 2B 实测统计

- Timeout：requested=5ms，actual=5.0031ms，completedDepth=1，nodes=1223
- Cancel：elapsed=124.366ms，completedDepth=2，nodes=26222
- Node Limit：nodes=200，completedDepth=1
- Iterative Deepening：completedDepth=3，nodes=27384
- 最大搜索深度：3
- 失败案例：无

## 尚未覆盖（Phase 2C）

- ZobristHash
- TranspositionTable
- Move Ordering 回归
## Phase 2C 结果

| 测试 | 结果 |
| --- | --- |
| Test 11a: Zobrist same board same hash | PASS |
| Test 11b: Zobrist move changes hash | PASS |
| Test 11c: Zobrist undo restores hash | PASS |
| Test 12a: TT store/probe | PASS |
| Test 12b: TT clear | PASS |
| Test 13: TT+MO regression legal/win | PASS |
| Test 14: Default config (TT+ID+MO) regression | PASS |

```text
Passed: 19
Failed: 0
```

## Phase 2C 实测统计

- Iterative Deepening：completedDepth=3，nodes=7261
- 默认配置回归：completedDepth=3，nodes=7505
- Cancel：elapsed=131.478ms，completedDepth=4，nodes=23198
- Timeout：requested=5ms，actual=5.0013ms
- 失败案例：无

## Phase 2 最终状态

```text
PHASE 2 = COMPLETE
AI CORE = TESTED
HUMAN VS HUMAN = PRESERVED
PHASE 3 = NOT STARTED
WAITING FOR NEXT INSTRUCTION
```

基线保护确认：

- `Gomoku` 主程序重新编译 PASS
- 双人对弈冒烟测试再次 PASS（启动、交替落子、黑方五连获胜、重新开始、重复落子拦截）
## Phase 3 结果（AIPlayer / HumanPlayer / GameController）

| 测试 | 结果 |
| --- | --- |
| Test 15a: Draw pattern has no five-in-row | PASS |
| Test 15b: Full board terminal no search | PASS |
| Test 16: AIPlayer returns legal move | PASS |
| Test 17: HumanPlayer roundtrip | PASS |
| Test 18: GameController HvH win | PASS |
| Test 19: GameController HvA alternating | PASS |
| Test 20: GameController AI first | PASS |

```text
Passed: 26
Failed: 0
```

## Phase 3 最终状态

```text
PHASE 3 = COMPLETE
AIPLAYER = TESTED
HUMANPLAYER = TESTED
GAME CONTROLLER = TESTED
HUMAN VS HUMAN = PRESERVED
PHASE 4 = NOT STARTED
```

基线保护确认：

- `Gomoku` 主程序重新编译 PASS
- 双人对弈冒烟测试再次 PASS
## Phase 4 结果（Human vs AI UI 接入）

| 验证项 | 结果 |
| --- | --- |
| Build | PASS |
| Human vs Human 冒烟测试 | PASS |
| Human vs AI（玩家先手） | PASS |
| Human vs AI（AI 先手） | PASS |

### Phase 4 实现内容

- `GameModeDialog`：双人对战 / 人机对战、玩家执子颜色、AI 难度
- `MainWindow`：异步 AI 调度（QtConcurrent + 快照 + 主线程回投）、AI 思考状态、AI 回合输入锁定、重开/退出取消
- `Game`：玩家类型、`isAITurn()`、`isHumanVsAI()`、状态快照、和棋检测
- `CMakeLists`：链接 Qt Concurrent 与 `ai_core`；由于当前环境 `g++ -E` 不可用导致 AutoMoc 的 `moc_predefs.h` 生成失败，改用手动 moc + CMake 自定义命令自动生成，已验证可复现构建

## 当前整体状态

```text
Build = PASS
Runtime = PASS
Human vs Human = PASS
Human vs AI = PASS
AI Core Tests = 26/26 PASS
```