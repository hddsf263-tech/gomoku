# AI 清理报告

**日期**: 2026-09-01
**阶段**: Cleanup Phase（已获用户确认）
**执行前恢复点**: `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku_cleanup_backup_20260901_200248`

---

## 1. 唯一工作目录

```text
D:\gomoku
```

今后所有开发、编译、测试、Codex 操作均以 `D:\gomoku` 为准。

## 2. C 盘目录处理方式

`C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku` 保留为历史参考目录，未删除。

已在其中新增：

```text
ARCHIVED_DO_NOT_DEVELOP_HERE.md
```

内容明确：该目录不是当前开发目录，当前唯一开发目录为 `D:\gomoku`。

## 3. 删除文件

本次未删除任何源代码或文档文件。

`D:\gomoku\build` 旧构建目录在恢复点建立后删除并重新生成，属于构建产物重建，不是源码删除。

## 4. 归档文件

以下文件从 `D:\gomoku` 移入 AI 历史归档：

| 原路径 | 归档路径 | 原因 |
| --- | --- | --- |
| `include\LocalAIPlayer.h` | `include\LocalAIPlayer.h` | 独立 AI 实现，未被 Game 使用 |
| `include\IAIPlayer.h` | `include\IAIPlayer.h` | 仅被 LocalAIPlayer 使用 |
| `include\LocalAIPlayer.h.bak` | `include\LocalAIPlayer.h.bak` | LocalAIPlayer 旧版备份 |
| `include\AIPlayer.h` | `include\AIPlayer.h` | 第二代 AI 封装 |
| `include\SearchEngine.h` | `include\SearchEngine.h` | 第二代 AI 搜索引擎 |
| `include\ZobristHash.h` | `include\ZobristHash.h` | AI 哈希模块 |
| `include\TranspositionTable.h` | `include\TranspositionTable.h` | AI 置换表模块 |
| `src\LocalAIPlayer.cpp` | `src\LocalAIPlayer.cpp` | 独立 AI 实现 |
| `src\AIPlayer.cpp` | `src\AIPlayer.cpp` | AI 封装实现 |
| `src\SearchEngine.cpp` | `src\SearchEngine.cpp` | D 盘当前版搜索引擎（isGameOver 未修复版） |
| `src\ZobristHash.cpp` | `src\ZobristHash.cpp` | AI 哈希实现 |
| `src\TranspositionTable.cpp` | `src\TranspositionTable.cpp` | AI 置换表实现 |
| `CMakeLists.txt.bak` | `scripts\CMakeLists.txt.bak` | 修复过程备份 |
| `fix.ps1` | `scripts\fix.ps1` | 一次性修复脚本 |
| `fix_cmake.ps1` | `scripts\fix_cmake.ps1` | 一次性修复脚本 |
| `audit_test.cpp` | `tests\audit_test.cpp` | AI 测试程序 |
| `audit_test.exe` | `tests\audit_test.exe` | AI 测试程序产物 |
| `benchmark_test.cpp` | `tests\benchmark_test.cpp` | AI 基准测试 |
| `benchmark_simple.cpp` | `tests\benchmark_simple.cpp` | AI 简化基准测试 |
| `benchmark_simple.exe` | `tests\benchmark_simple.exe` | AI 基准测试产物 |

归档目录：

```text
C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku_ai_archive_20260901
```

## 5. 保留的 AI 历史文件

- `AIPlayer.h/.cpp`、`SearchEngine.h/.cpp`、`ZobristHash.h/.cpp`、`TranspositionTable.h/.cpp`：全部保留在 AI 历史归档中，不参与当前构建。
- C 盘原目录完整保留，其中的 `SearchEngine.cpp` 是 `isGameOver()` 修复版。
- 归档中额外保存 `src\SearchEngine.cpp.fixed_from_C_20260901`，即 C 盘修复版副本，供后续 AI 重建参考。
- `SearchEngine.cpp` 两个版本的处理方式：
  - D 盘版本：归档为 `src\SearchEngine.cpp`；
  - C 盘版本：保留在原目录，并复制到归档 `src\SearchEngine.cpp.fixed_from_C_20260901`；
  - 迁移价值判断见 `docs/AI_MIGRATION_PREFLIGHT.md`，结论为仅 `isGameOver()` 修复值得迁移。
- AI 历史文档保留在 `D:\gomoku` 根目录与 `docs/`（如 `AI_CAPABILITY.md`、`LOCALAI_V1.5_CHANGES.md`、`PROJECT_RECOVERY_REPORT.md`、`docs/AI_*`）。

## 6. 工程引用变化

- `CMakeLists.txt`：移除 7 个 AI 源/头文件引用，仅保留基础双人对弈与精简模式对话框；版本号改为 1.0。
- `include/Game.h`、`src/Game.cpp`：移除 `AIPlayer` 指针、`setPlayerType()`、`isAITurn()`、`makeAIMove()`、`getAIMove()`、`isHumanVsHuman()`、`isHumanVsAI()`、`setAIMaxDepth()`。
- `src/MainWindow.h`、`src/MainWindow.cpp`：移除 AI 异步搜索、AI 回合触发、AI 思考状态、模式配置函数和 `onAIMoveReady`。
- `src/GameModeDialog.h/.cpp`：精简为仅“双人对战”模式。
- `include/ChessPiece.h`：保留 `PlayerType` 枚举定义，不包含任何 AI 逻辑。
- 重新配置后 `build/build.ninja` 不再包含任何 AI 文件。

## 7. CMake 构建结果

```text
Build：PASS
```

- 编译器：MinGW GCC 13.1.0
- Qt：6.11.2
- CMake：3.30.5 + Ninja
- 编译目标：9/9 成功
- 可执行文件：`D:\gomoku\build\Gomoku.exe`
- 文件大小：152,769 字节
- 生成时间：2026-09-01 20:06:39

## 8. Gomoku.exe 启动结果

```text
程序：可以正常启动
```

通过 UI 自动化启动程序并确认主窗口出现。

## 9. 双人对弈测试结果

测试脚本：`docs/HVH_SMOKE_TEST.ps1`

执行流程与结果：

| 测试项 | 结果 |
| --- | --- |
| 启动程序 | PASS |
| 新游戏对话框 | PASS |
| 玩家1 落子 | PASS |
| 玩家2 落子 | PASS |
| 黑白交替 | PASS |
| 持续落子 | PASS |
| 黑方五连获胜 | PASS |
| 胜负弹窗 | PASS |
| 重新开始 | PASS |
| 重新开始后再次落子 | PASS |
| 重复落子拦截 | PASS |

实际输出摘要：

```text
MOVE 7,7 -> 白方回合 | 游戏进行中
MOVE 8,8 -> 黑方回合 | 游戏进行中
...
MOVE 7,11 -> 黑方获胜！ | 黑方获胜! | 游戏结束
AFTER_RESTART_MOVE=白方回合 | 游戏进行中
```

## 10. 当前状态

```text
基础双人对弈工程已清理完成。

D:\gomoku 为唯一开发目录。

旧 AI 不再作为当前工程的一部分。

AI 人机对弈尚未重新实现。
```

## 11. 恢复点与痕迹

- 完整恢复点：`C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku_cleanup_backup_20260901_200248`（149 个文件，67.76 MB）
- AI 归档：`C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku_ai_archive_20260901`（21 个文件）
- 审计报告：`docs/AI_CLEANUP_CHECKPOINT.md`
- 执行日志：`docs/AI_CLEANUP_EXECUTION_LOG.md`
- 迁移预检：`docs/AI_MIGRATION_PREFLIGHT.md`
- UI 自动化脚本：`docs/UIA_DIAG.ps1`、`docs/HVH_SMOKE_TEST.ps1`

说明：由于沙箱无法通过补丁工具读取 D 盘已有文件，本次源码修改采用受控整文件重写方式完成；完整修改范围已在本报告第 6 节记录，备份与归档可完整恢复清理前状态。
