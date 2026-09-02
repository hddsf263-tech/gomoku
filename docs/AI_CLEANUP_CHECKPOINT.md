# AI 清理检查点报告（NO-DELETE CHECKPOINT）

**日期**: 2026-09-01
**阶段**: 只读审计（本阶段未修改、删除、移动、重命名任何文件）
**审计对象**: `D:\gomoku` 与 `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku`

> 本报告只做分类和建议。所有 DELETE_CANDIDATE / ARCHIVE_CANDIDATE 均未执行，必须等待人工确认。

---

## 1. 两个目录状态

### 目录 A：`D:\gomoku`

| 检查项 | 结果 |
| --- | --- |
| 项目完整性 | 完整：源码、CMake 工程、构建输出、测试程序、历史文档都在 |
| 能否构建 | 能。`build/Gomoku.exe` 生成于 2026-09-01 19:32:39，晚于当前全部源码修改时间 |
| 构建配置 | `build.ninja`、`CMakeCache.txt` 均指向 `D:/gomoku`；Release + Ninja + Qt 6.11.2 + MinGW 13.1.0 |
| 主要源码 | main、MainWindow、BoardWidget、Game、GameModeDialog、Board，以及 ZobristHash、TranspositionTable、SearchEngine、AIPlayer、LocalAIPlayer |
| AI 文件 | 两套并存：`AIPlayer + SearchEngine`（Game 实际使用）；`LocalAIPlayer + IAIPlayer`（已编译但未被 Game 使用） |
| 重复文件 | `LocalAIPlayer.h.bak`、`CMakeLists.txt.bak`；根目录与 `docs/` 各有 `AI_CAPABILITY.md` |
| 历史记录 | `PROJECT_STATE.md`、`PROJECT_RECOVERY_REPORT.md`、`LOCALAI_V1.5_CHANGES.md`、`docs/AI_INTEGRATION_*.md` |
| 状态结论 | 当前唯一具有完整可运行构建的目录 |

### 目录 B：`C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku`

| 检查项 | 结果 |
| --- | --- |
| 项目完整性 | 基本完整，但缺少 `GameModeDialog`、`IAIPlayer`、`LocalAIPlayer`、测试脚本 |
| 能否构建 | 配置过，但 `build/` 内没有 `Gomoku.exe`；CMakeCache 的 `CMAKE_BUILD_TYPE` 为空 |
| 构建配置 | `CMakeCache.txt` 指向 `C:/Users/Lenovo/Documents/ChatGPT/杂项事务/gomoku`，属于旧配置 |
| 主要源码 | 基础双人对弈 + `AIPlayer/SearchEngine/ZobristHash/TranspositionTable` |
| AI 文件 | `MainWindow.cpp` 没有模式选择入口，人类落子后不触发 AI；`Game` 虽含 AI 方法但 UI 无法使用 |
| 历史记录 | `PROJECT_STATE.md`、`docs/PROJECT_STATUS_AUDIT_20260901.md`、`docs/PROJECT_HANDOVER_AUDIT.md`、`docs/LOCALAI_V1.5_AUDIT.md` |
| 关键差异 | `src/SearchEngine.cpp` 是已修复 `isGameOver()` 的版本，与 D 盘当前未修复版本不一致 |
| 状态结论 | 旧源码快照 / 历史工作区，不是当前实际运行工程 |

---

## 2. 推荐的唯一工作目录

```text
RECOMMENDED_WORKSPACE:
D:\gomoku
```

选择依据：

1. 当前实际运行工程：只有 D 盘存在最新 `Gomoku.exe`。
2. 工程配置完整：`CMakeLists.txt` 当前语法有效，构建产物与源码匹配。
3. 源码与构建系统匹配：`build.ninja` 显示所有源文件均来自 `D:/gomoku`。
4. 基础双人对弈可构建：双人对战仍是默认模式，构建链完整。
5. PROJECT/STATE 历史：D 盘的恢复与集成记录比 C 盘更完整、更靠后。
6. Codex 实际开发主目录：2026-09-01 傍晚的恢复、模式对话框、异步 AI 修改都发生在 D 盘。
7. Git 历史：D 盘无 `.git`；C 盘位于父级未提交 Git 仓库内，两者都没有有效提交记录。
8. 长期唯一目录：D 盘具备源码、测试程序、构建输出和 Qt 运行时，适合作为唯一开发目录。

建议：`C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku` 后续整体归档为历史备份，不再作为开发目录。

---

## 3. 基础双人对弈文件清单

| 文件 | 类型 | 作用 | 当前工程是否引用 | 是否必须保留 |
| --- | --- | --- | --- | --- |
| `CMakeLists.txt` | 构建配置 | 工程构建入口 | 是 | 必须保留（清理阶段再修正 AI 引用） |
| `src/main.cpp` | 源码 | 程序入口 | 是 | 必须保留 |
| `src/MainWindow.h/.cpp` | UI | 主窗口、菜单、模式入口 | 是 | 必须保留（当前含 AI 代码，需清理） |
| `src/BoardWidget.h/.cpp` | UI | 棋盘绘制与鼠标落子 | 是 | 必须保留 |
| `include/Game.h` + `src/Game.cpp` | 逻辑 | 回合、落子、胜负、悔棋 | 是 | 必须保留（当前含 AI 成员，需清理） |
| `include/Board.h` + `src/Board.cpp` | 数据 | 棋盘状态、五连判断 | 是 | 必须保留 |
| `include/ChessPiece.h` | 枚举 | 棋子、状态、玩家类型 | 是 | 必须保留 |
| `build.ps1` | 脚本 | 快速构建 | 是 | 必须保留 |
| `README.md` | 文档 | 项目说明（v1.0） | 是 | 保留 |
| `docs/软件设计文档.md` | 文档 | 课程设计文档 | 否 | 保留 |
| `docs/环境安装指南.md` | 文档 | 环境安装说明 | 否 | 保留 |
| `docs/screenshot_main.png` | 资源 | 界面截图 | 否 | 保留 |
| `五子棋项目开题答辩 PPT.pptx` | 资料 | 课程开题 PPT | 否 | 保留 |
| `docs/开题 PPT 说明.md` | 资料 | 开题说明 | 否 | 保留 |

---

## 4. 历史正常 AI 文件清单

| 文件 | 路径 | 所属版本 | 作用 | 证据 | 当前是否引用 |
| --- | --- | --- | --- | --- | --- |
| `IAIPlayer.h` | `include/` | LocalAI v1.0/v1.5 | AI 抽象接口 | `docs/AI_CAPABILITY.md` 描述接口设计 | 仅被 LocalAIPlayer 使用 |
| `LocalAIPlayer.h/.cpp` | `include/`、`src/` | LocalAI v1.0/v1.5 | 棋型识别、候选点、Minimax + Alpha-Beta 的独立 AI | `docs/AI_CAPABILITY.md` 记录测试 A-E 通过；`PROJECT_RECOVERY_REPORT.md` 确认实现完整但未被 Game 使用 | CMake 编译，Game 未使用 |
| `LocalAIPlayer.h.bak` | `include/` | LocalAI v1.0 | 旧版 LocalAIPlayer 备份 | 文件时间戳 2026-08-31 15:24 | 否 |
| `ZobristHash.h/.cpp` | `include/`、`src/` | LocalAI v1.5 | 64 位棋盘哈希 | 本次运行 `audit_test.exe` 通过 Zobrist 一致性测试 | 被 SearchEngine 使用 |
| `TranspositionTable.h/.cpp` | `include/`、`src/` | LocalAI v1.5 | 置换表 | 本次运行 `audit_test.exe` 通过 TT 存储/查询测试 | 被 SearchEngine 使用 |
| `SearchEngine.h/.cpp` | `include/`、`src/` | LocalAI v1.5 | Negamax、Alpha-Beta、迭代加深、着法排序 | `PROJECT_STATE.md`、`LOCALAI_V1.5_CHANGES.md`；本次运行审计测试达到深度 4 | 被 AIPlayer 使用 |
| `AIPlayer.h/.cpp` | `include/`、`src/` | LocalAI v1.5 | AI 玩家封装 | `audit_test.cpp`、`benchmark_*.cpp` 均直接调用 | 被 Game 使用 |
| `GameModeDialog.h/.cpp` | `src/` | LocalAI v1.5 恢复版 | 人机/双人模式、执子颜色、AI 难度选择 | `MainWindow::onNewGame()` 调用；2026-09-01 构建通过 | 被 MainWindow 使用 |
| `audit_test.cpp`、`benchmark_test.cpp`、`benchmark_simple.cpp` | 根目录 | LocalAI v1.5 | AI 功能审计与性能基准 | 本次运行 `audit_test.exe` 输出 PASS | 否（独立测试程序） |

历史上“能够正常运行的人机对弈”由两代实现组成：

1. 第一代：`IAIPlayer + LocalAIPlayer`，以棋型识别和静态评估为主，文档记录测试通过。
2. 第二代：`AIPlayer + SearchEngine + ZobristHash + TranspositionTable`，加入 TT、迭代加深、Alpha-Beta 和着法排序；本次只读运行 `audit_test.exe` 验证核心模块 PASS。

注意：GUI 人机对弈是否可用属于运行时验证，本检查点阶段未打开界面进行人工操作，需要下一阶段实测。

---

## 5. 废弃 AI 文件候选清单

建议值仅用于后续清理阶段，本阶段不执行。

| 文件 | 路径 | 判断原因 | 与哪个版本相关 | 当前是否引用 | 建议 |
| --- | --- | --- | --- | --- | --- |
| `LocalAIPlayer.h` | `include/` | 独立 AI 实现，Game 使用的是 AIPlayer，属于重复路线 | v1.0/v1.5 | 仅 CMake 编译 | DELETE_CANDIDATE / ARCHIVE_CANDIDATE |
| `LocalAIPlayer.cpp` | `src/` | 同上，未被任何业务代码调用 | v1.0/v1.5 | 仅 CMake 编译 | DELETE_CANDIDATE / ARCHIVE_CANDIDATE |
| `IAIPlayer.h` | `include/` | 仅被 LocalAIPlayer 继承；LocalAIPlayer 废弃后失去引用 | v1.0/v1.5 | 否（间接） | DELETE_CANDIDATE / ARCHIVE_CANDIDATE |
| `LocalAIPlayer.h.bak` | `include/` | 旧版备份，重复 | v1.0 | 否 | ARCHIVE_CANDIDATE / DELETE_CANDIDATE |
| `CMakeLists.txt.bak` | 根目录 | 修复过程的备份，且旧版存在 include 路径缺空格等问题 | 修复过程 | 否 | ARCHIVE_CANDIDATE |
| `fix.ps1`、`fix_cmake.ps1` | 根目录 | 一次性修复脚本 | 修复过程 | 否 | ARCHIVE_CANDIDATE / DELETE_CANDIDATE |
| `audit_test.cpp/.exe`、`benchmark_test.cpp/.exe`、`benchmark_simple.cpp/.exe` | 根目录 | AI 专用测试/基准程序，不属于基础双人对弈 | v1.5 | 否 | ARCHIVE_CANDIDATE |
| `SearchEngine.cpp`（D 盘当前版） | `src/` | `isGameOver()` 仍返回 false，与 C 盘修复版冲突 | v1.5 | 是（当前构建） | UNCERTAIN：清理阶段需采用 C 盘修复版或重新实现 |
| `AI_CAPABILITY.md`（根目录与 docs/ 各一份） | 根目录、docs/ | AI 能力文档重复 | v1.0/v1.5 | 否 | KEEP（保留历史） |
| `LOCALAI_V1.5_CHANGES.md`、`docs/AI_PERFORMANCE.md`、`docs/AI_INTEGRATION_*.md`、`PROJECT_RECOVERY_REPORT.md`、`PROJECT_STATE.md` | 根目录、docs/ | AI 历史记录 | v1.0/v1.5 | 否 | KEEP（保留历史） |
| `C:\Users\Lenovo\Documents\ChatGPT\杂项事务\gomoku` 整个目录 | 目录 | 旧源码快照，与 D 盘重复且不完整 | v1.0-v1.5 | 否 | ARCHIVE_CANDIDATE（目录级，禁止直接删除） |

---

## 6. 工程引用风险

- D 盘 `CMakeLists.txt` 仍编译 `LocalAIPlayer.cpp` 并登记 `IAIPlayer.h`、`LocalAIPlayer.h`，但没有业务代码调用，属于冗余编译。
- `LocalAIPlayer.h` 在 `Gomoku` 命名空间内重复声明了 `ZobristHash`、`TranspositionTable`、`SearchEngine`，与 `include/` 下同名类冲突；同一编译单元同时包含时会重定义。
- `SearchEngine.cpp` 两盘不一致：D 盘当前构建使用未修复版，C 盘保留已修复版。清理阶段必须明确取舍，否则基础 AI 清理和后续重建会继续踩坑。
- C 盘 `MainWindow.cpp` 没有模式选择，也没有在人类落子后触发 AI；若把 C 盘当唯一目录，会丢掉 D 盘已经恢复的人机入口。
- C 盘 `build/` 无 `Gomoku.exe`，且 CMakeCache 的构建类型为空；不能作为当前运行工程。
- 两处都没有有效 Git 提交：D 盘无 `.git`，C 盘在父级仓库中但无 commit，历史只能依靠文档与时间戳。
- `MainWindow` 中声明并实现了 `onAIMoveReady`，但当前异步 watcher 未连接该槽，属于死代码。
- `resources/` 目录在两个工程中均为空，属于可整理项。

---

## 7. 证据链摘要

- 文件哈希对比：`Board`、`ChessPiece`、`BoardWidget`、`main.cpp`、`README.md`、`build.ps1`、`ZobristHash`、`TranspositionTable`、`SearchEngine.h`、`AIPlayer.h/.cpp` 等在两目录中完全相同。
- `SearchEngine.cpp` 差异：`fc /n` 对比确认 D 版与 C 版仅 `isGameOver()` 实现不同（D 未修复，C 已修复）。
- 构建证据：`D:\gomoku\build\build.ninja` 列出全部 13 个编译目标，含 `GameModeDialog` 与 `LocalAIPlayer`；`CMakeCache.txt` 的 `CMAKE_HOME_DIRECTORY` 为 `D:/gomoku`。
- 运行证据：本次只读运行 `D:\gomoku\audit_test.exe`，Zobrist、Board 拷贝、TT、迭代加深全部 PASS，搜索统计约 197,954 节点。
- Git 证据：`D:\gomoku` 无 `.git`；C 盘 gomoku 目录本身无 `.git`，git 顶层位于 `C:\Users\Lenovo\Documents\ChatGPT\杂项事务`，且没有任何提交。

---

## 8. NO-DELETE CHECKPOINT REACHED

```text
NO-DELETE CHECKPOINT REACHED

当前阶段：
只读审计已完成

文件删除：0
文件移动：0
文件重命名：0
源代码修改：0
工程配置修改：0
AI 重建：未开始
```

在用户明确确认检查点报告之前，本任务不得执行任何删除、移动、重命名、覆盖、清空目录或工程引用移除操作。

允许的确认指令示例：

```text
确认清理
```

或：

```text
Proceed with cleanup
```

---

## 9. 下一阶段（Cleanup）风险提示

1. 若废弃 `LocalAIPlayer`，必须同步从 `CMakeLists.txt` 移除 `src/LocalAIPlayer.cpp`、`include/IAIPlayer.h`、`include/LocalAIPlayer.h` 的引用。
2. 若确认 D 盘为唯一工作目录，应先解决 `SearchEngine.cpp` 的 `isGameOver()` 版本冲突，再重新构建。
3. 清理 AI 时必须保留 `Board`、`Game`、`MainWindow`、`BoardWidget`、`main.cpp` 等基础文件，只剥离 AI 相关引用。
4. 清理后必须重新编译并完成至少一轮实际双人对弈测试。
5. C 盘目录应先整体归档，确认无遗漏后再考虑后续处理，避免误删历史。
