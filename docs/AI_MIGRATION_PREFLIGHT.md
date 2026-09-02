# AI 迁移价值预检报告（Cleanup Preflight）

**日期**: 2026-09-01
**阶段**: NO-DELETE（只读分析，未修改、删除、移动、重命名任何文件）
**关联文档**: `docs/AI_CLEANUP_CHECKPOINT.md`、`docs/AI_CLEANUP_EXECUTION_LOG.md`

> 本报告回答一个核心问题：C 盘目录里是否存在应该迁移到 `D:\gomoku` 的有效修复。

---

## 1. SearchEngine.cpp 精确差异分析

### 1.1 函数清单（两版完全一致）

| 函数 | D 盘行号 | C 盘行号 |
| --- | --- | --- |
| `SearchEngine()` 构造 | 7 | 7 |
| `findBestMove()` | 16 | 16 |
| `negamax()` | 83 | 83 |
| `generateMoves()` | 167 | 167 |
| `sortMoves()` | 181 | 181 |
| `getHeuristicScore()` | 197 | 197 |
| `evaluate()` | 220 | 220 |
| `isGameOver()` | 244 | 244 |

### 1.2 逐行差异

`fc /n` 对比结果确认：除文件末尾空行外，两版 `SearchEngine.cpp` 只有 `isGameOver()` 一处实现不同。

D 盘当前版：

```cpp
bool SearchEngine::isGameOver(const Board& board, int& winner) {
    winner = 0;
    return false;
}
```

C 盘修复版：

```cpp
bool SearchEngine::isGameOver(const Board& board, int& winner) {
    auto lastMove = board.getLastMove();
    if (lastMove.has_value()) {
        ChessPiece piece = board.getPiece(lastMove->row, lastMove->col);
        if (piece != ChessPiece::Empty && board.checkFiveInRow(lastMove->row, lastMove->col, piece)) {
            winner = (piece == ChessPiece::Black) ? 1 : -1;
            return true;
        }
    }
    winner = 0;
    return false;
}
```

### 1.3 D 盘版本问题

- `isGameOver()` 永远返回 `false`，因此 `negamax()` 第 88 行的终局判断永远不会触发。
- 即使棋盘已经形成五连，AI 搜索仍会继续展开，无法在搜索中识别胜局或败局。
- 后果是 AI 可能继续搜索本应结束的局面，浪费节点和时间，且无法利用“必胜/必败”结果做正确剪枝。
- 该函数当前被 D 盘工程使用，调用点是 `SearchEngine::negamax()`，当前 `Gomoku.exe` 内编译的就是这个未修复版本。

### 1.4 C 盘修复内容

- 改为检查 `board.getLastMove()`，即最近落子位置。
- 通过 `board.getPiece()` 取最近落子的棋子。
- 调用 `board.checkFiveInRow()` 判断是否形成五连。
- 五连成立时按黑棋=1、白棋=-1 写入 `winner` 并返回 `true`。

### 1.5 修复逻辑是否正确

正确。原因：

1. 搜索过程中每次落子后都会递归调用 `negamax()`，此时“最后一步”恰好是本次搜索刚生成的着法。
2. `winner` 的符号约定与 `negamax()` 的颜色参数一致：黑棋 `color=1`，白棋 `color=-1`。
3. `negamax()` 中的判断 `winner == ((color == 1) ? 1 : -1)` 能正确区分己方获胜与对方获胜。

### 1.6 与 D 盘当前代码是否兼容

兼容。依据：

- 两盘 `include/SearchEngine.h` 完全一致，函数签名相同。
- 两盘 `include/Board.h`、`src/Board.cpp`、`include/ChessPiece.h` 完全一致，`getLastMove()`、`getPiece()`、`checkFiveInRow()` 均存在。
- 两盘 `AIPlayer.h/.cpp` 完全一致，调用链不变。
- 修复只改变一个函数体，不改变公开接口，不影响其他模块。

### 1.7 历史测试证据

- `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku\PROJECT_STATE.md`：任务队列记录“isGameOver() Bug 修复 ✅ 完成，2026-09-01 已修复并编译”。
- `docs/PROJECT_STATUS_AUDIT_20260901.md`：记录 2026-09-01 16:52 修改 `SearchEngine.cpp`，16:55 重新编译成功。
- `docs/LOCALAI_V1.5_AUDIT.md`：记录修复后基准测试中出现“活三/冲四/一步获胜”局面节点数异常低，并解释为终局检测提前返回，即修复生效。
- `docs/PROJECT_HANDOVER_AUDIT.md`：记录“修复可能只存在于另一个分支/未同步回工作区”，与当前两盘不同步现象吻合。
- 未找到 `TEST_RESULT.md`、`FINAL_QA.md`、`CHANGELOG`、`TASK.md` 这类独立测试报告文件；现有“测试通过”均来自审计文档描述，没有独立自动化断言覆盖 `isGameOver()`。

### 1.8 为什么 D 盘没有同步

时间线证据：

- C 盘 `SearchEngine.cpp` 修改时间：2026-09-01 16:52（修复版）。
- D 盘 `SearchEngine.cpp` 修改时间：2026-09-01 16:13（未修复版）。
- D 盘后续 AI 恢复工作发生在 17:48-19:32，主要集中在 `CMakeLists.txt`、`MainWindow`、`Game`、`GameModeDialog`。
- 两个目录都不是有效 Git 提交基线，无法通过版本控制自动合并。

最可能原因：后续恢复过程基于 D 盘旧版 `SearchEngine.cpp` 继续开发，没有把 C 盘 16:52 的 `isGameOver()` 修复合并回来。

### 1.9 是否迁移及迁移范围

- 建议：**迁移**，并且**只迁移 `isGameOver()` 这一个函数修复**，不要整文件覆盖、不要复制整个 C 盘 AI 目录。
- 理由：两版其余函数完全一致，整文件替换收益为零且增加风险；C 盘其余 AI 核心文件与 D 盘哈希一致，无其他需要迁移的内容。
- 执行时机：只能在用户确认清理指令之后执行，本阶段不修改任何源代码。

---

## 2. 全部 AI 相关文件差异表

| 文件 | D 盘状态 | C 盘状态 | 差异性质 | C 盘版本是否更可靠 | 是否建议迁移 |
| --- | --- | --- | --- | --- | --- |
| `src/SearchEngine.cpp` | 未修复 `isGameOver()` | 修复版 | 功能修复 | 是（该函数） | 是（仅此函数） |
| `include/SearchEngine.h` | 正常 | 正常 | 无差异 | - | 否 |
| `src/AIPlayer.cpp` | 正常 | 正常 | 无差异 | - | 否 |
| `include/AIPlayer.h` | 正常 | 正常 | 无差异 | - | 否 |
| `src/ZobristHash.cpp` | 正常 | 正常 | 无差异 | - | 否 |
| `include/ZobristHash.h` | 正常 | 正常 | 无差异 | - | 否 |
| `src/TranspositionTable.cpp` | 正常 | 正常 | 无差异 | - | 否 |
| `include/TranspositionTable.h` | 正常 | 正常 | 无差异 | - | 否 |
| `include/Game.h` | 更完整（含 getAIMove、isHumanVsAI 等） | 旧版 | 功能演进 | 否 | 否（保留 D 版） |
| `src/Game.cpp` | 更完整 | 旧版 | 功能演进 | 否 | 否（保留 D 版） |
| `src/MainWindow.h` | 更完整（模式对话框、AI 异步状态） | 旧版 | 功能演进 | 否 | 否（保留 D 版） |
| `src/MainWindow.cpp` | 更完整（人机入口、异步 AI） | 旧版 | 功能演进 | 否 | 否（保留 D 版） |
| `CMakeLists.txt` | 当前有效构建配置 | 旧版 | 构建配置演进 | 否 | 否（保留 D 版） |

结论：C 盘所有 AI 核心文件中，只有 `SearchEngine.cpp` 的 `isGameOver()` 修复具有迁移价值；C 盘其余文件不是更新，而是旧版。

---

## 3. 历史记录检索结果

### 3.1 已找到

- `D:\gomoku\PROJECT_STATE.md`
- `D:\gomoku\PROJECT_RECOVERY_REPORT.md`
- `D:\gomoku\LOCALAI_V1.5_CHANGES.md`
- `C:\...\gomoku\PROJECT_STATE.md`
- `C:\...\gomoku\docs\PROJECT_STATUS_AUDIT_20260901.md`
- `C:\...\gomoku\docs\PROJECT_HANDOVER_AUDIT.md`
- `C:\...\gomoku\docs\LOCALAI_V1.5_AUDIT.md`
- 两份 `AI_CAPABILITY.md`、`docs/AI_PERFORMANCE.md`

### 3.2 未找到

- `TASK.md`
- `TEST_RESULT.md`
- `FINAL_QA.md`
- `CHANGELOG`
- 其他独立的 v2/v3 AI 版本记录

### 3.3 与 isGameOver 直接相关的记录

- `PROJECT_STATE.md`（C 盘）：TASK 1 标记“isGameOver() Bug 修复 ✅ 完成”。
- `PROJECT_STATUS_AUDIT_20260901.md`：明确写出修复前/修复后代码和编译时间。
- `LOCALAI_V1.5_AUDIT.md`：写明修复生效的测试观察。
- `PROJECT_HANDOVER_AUDIT.md`：指出修复未同步到源代码工作区的风险。

---

## 4. 清理前最终决策分类

### A. 可以直接废弃的文件（候选）

- `include/LocalAIPlayer.h`
- `src/LocalAIPlayer.cpp`
- `include/IAIPlayer.h`
- `include/LocalAIPlayer.h.bak`
- `CMakeLists.txt.bak`
- `fix.ps1`、`fix_cmake.ps1`
- `audit_test.cpp/.exe`、`benchmark_test.cpp/.exe`、`benchmark_simple.cpp/.exe`

说明：以上为 AI 重复路线、备份或一次性修复/测试产物，正式清理时建议先归档再决定是否删除。

### B. 可以归档的文件

- `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku` 整个目录（历史参考）
- C 盘版本的 `SearchEngine.cpp` 应先单独留存修复记录，再整体归档
- D 盘 AI 历史文档（`LOCALAI_V1.5_CHANGES.md`、`docs/AI_INTEGRATION_*.md`、`PROJECT_RECOVERY_REPORT.md` 等）

### C. 建议从 C 盘迁移到 D 盘的文件/修复

- 唯一项：`SearchEngine.cpp` 中 `isGameOver()` 的修复逻辑

迁移方式建议：把 C 盘修复代码整理为补丁说明并纳入 D 盘 AI 重建基线；不要在未确认前直接复制整文件。

### D. 必须人工确认的文件

- `src/SearchEngine.cpp`：是否采用 C 盘修复版、何时应用
- C 盘目录：归档位置与是否保留
- `include/Game.h`、`src/Game.cpp`、`src/MainWindow.h`、`src/MainWindow.cpp`：确认保留 D 盘更完整版本
- `CMakeLists.txt`：确认 D 盘当前配置作为唯一构建配置

### E. 不应该动的基础双人对弈文件

- `src/main.cpp`
- `src/BoardWidget.h/.cpp`
- `include/Board.h`、`src/Board.cpp`
- `include/ChessPiece.h`
- `README.md`
- `build.ps1`
- `docs/软件设计文档.md`、`docs/环境安装指南.md`
- `Game`、`MainWindow`、`BoardWidget` 中与双人对弈、胜负判断、悔棋、界面相关的基础逻辑

---

## 5. 最终状态

```text
CLEANUP PREFLIGHT COMPLETE

文件删除：0
文件移动：0
文件重命名：0
源代码修改：0
工程配置修改：0
AI 重建：0

D:\gomoku：
最终唯一工作目录

C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku：
历史参考目录

当前状态：
WAITING FOR USER CONFIRMATION
```

本阶段未执行删除、移动、重命名、源码修改、工程配置修改或代码迁移。下一步清理仍需用户明确确认。

---

## 6. 本预检执行痕迹

- 对比工具：`fc /n`、SHA256 哈希、`rg`
- 对比范围：两版 `SearchEngine.cpp`、全部 AI 核心文件、历史文档
- 运行测试：`D:\gomoku\audit_test.exe`（Zobrist、TT、迭代加深 PASS）
- 新增产物：本文件 `docs/AI_MIGRATION_PREFLIGHT.md`
- 修改/删除：0
