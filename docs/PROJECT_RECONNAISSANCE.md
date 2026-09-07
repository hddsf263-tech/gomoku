# Gomoku Project Reconnaissance

> 本次勘察基于对整个远程仓库 `https://github.com/hddsf263-tech/gomoku` 的**完整通读**（通过临时只读克隆到 `C:\Users\Lenovo\.codex\visualizations\2026\09\07\01a07971-52a6-7c63-b0e6-a48f38f92ef1\gomoku_remote`），并以本地 `D:\gomoku` 作对比。
> 全程只读：未修改任何源代码、未提交、未推送。

## 1. Audit Scope

- 通读对象：远程仓库 `hddsf263-tech/gomoku` 的 `main` 分支（唯一分支）。
- 读取方式：`git clone --depth 1` 临时克隆，逐文件读取全部 65 个跟踪文件（源码 / 头文件 / CMake / docs / 测试 / index.html / PPT）。
- 对比对象：本地 `D:\gomoku`（其跟踪文件清单与远程几乎一致，但缺少 3 个文件，见第 16 节）。
- 未执行任何写操作。

## 2. Repository Information

- Git root（远程）：`https://github.com/hddsf263-tech/gomoku`
- Git root（本地）：`D:\gomoku`
- remote：`origin https://github.com/hddsf263-tech/gomoku.git`
- 分支：`main`
- 远程 HEAD：`2de5d6b`（`Upload Gomoku game`，作者 `ariennbkls666-cell`，2026-09-06 14:30:18 +0800）
- 远程提交数：**仅 1 个**（一次性整体上传，无历史演进）
- 本地 HEAD：`0df97e7`（`chore: initialize gomoku project`）
- 本地工作树：**clean**

  > 说明：本地与远程各自只有一个提交，但 commit 不同（远程 `2de5d6b`，本地 `0df97e7`）。两者是**独立的单快照仓库**，本地并非远程的 git-history 演进关系。

## 3. Project Structure

```
gomoku/
├── index.html                     # 单文件网页五子棋（2272 行，独立可运行）
├── CMakeLists.txt                 # Qt 桌面工程构建
├── build.ps1                      # Windows 构建脚本（Qt6 + MinGW + Ninja）
├── README.md                      # 仅描述 index.html 网页版（过时）
├── PROJECT_STATE.md               # 声称 AI 重建 COMPLETE
├── AI_CAPABILITY.md               # v1.5 AI 能力说明（API 描述已过时）
├── LOCALAI_V1.5_CHANGES.md        # v1.5 变更记录（API 描述已过时）
├── PROJECT_RECOVERY_REPORT.md     # 历史恢复审计（描述旧双 AI 状态，已过时）
├── 五子棋项目开题答辩 PPT.pptx    # 课程开题 PPT
├── include/
│   ├── ChessPiece.h  Board.h  Game.h  GameController.h  NetworkManager.h
│   └── ai/ AIConfig.h GameStateSnapshot.h MoveGenerator.h Evaluation.h
│          SearchEngine.h SearchResult.h TranspositionTable.h ZobristHash.h
│          IPlayer.h AIPlayer.h HumanPlayer.h
├── src/
│   ├── main.cpp  MainWindow.h/.cpp  BoardWidget.h/.cpp  Game.cpp
│   ├── GameModeDialog.h/.cpp  Board.cpp  GameController.cpp  NetworkManager.cpp
│   └── ai/ AIPlayer.cpp HumanPlayer.cpp MoveGenerator.cpp Evaluation.cpp
│          SearchEngine.cpp TranspositionTable.cpp ZobristHash.cpp
├── tests/ai_core_tests.cpp        # 独立 AI + 控制器测试（27 项）
└── docs/                          # 大量 AI_* 历史/阶段报告 + 设计/环境指南 + 冒烟脚本 + 截图
```

## 4. Build System

- CMake `min 3.16`，`project(Gomoku VERSION 1.0 LANGUAGES CXX)`，C++17。
- 优先找 Qt6（Core/Gui/Widgets/Concurrent/Network），否则 Qt5.15 5.15。
- 目标：
  - `Gomoku`（GUI，链接 `ai_core` + Qt）
  - `ai_core`（静态库：AI 核心 + GameController）
  - `ai_core_tests`（独立测试，链 `ai_core`，不依赖 Qt Widgets）
- 特殊处理：因当前环境 `g++ -E` 不可用导致 AutoMoc 的 `moc_predefs.h` 生成失败，改为**手动 moc + CMake custom command**，并对三个带 `Q_OBJECT` 的类（MainWindow/BoardWidget/GameModeDialog）关掉 AUTOMOC。
- `build.ps1`：Qt 6.11.2 mingw_64、CMake/Ninja/MinGW 13.1.0，Release + Ninja。

## 5. Application Entry Point

```
src/main.cpp → QApplication → MainWindow window.show() → app.exec()
```
- 应用信息：org `GomokuTeam`，app `Gomoku`，version `1.0`。

## 6. Core Game Architecture

- `ChessPiece`（枚举 Empty/Black/White）、`GameState`（NotStarted/InProgress/BlackWin/WhiteWin/Draw）、`PlayerType`（Human/AI）。
- `Board`（15×15）：`grid`、`moveHistory`、`lastMove`；`placePiece/undoLastMove/checkFiveInRow`（4 方向双向计数），支持拷贝（供 AI 搜索）。
- `Game`：持有 `Board`、`currentPlayer`、`state`、`blackPlayerType/whitePlayerType`、回调；`makeMove` 校验→落子→`checkGameEnd`（五连/满盘和棋）→切换玩家；`undoMove`；`isAITurn/isHumanVsAI/getGameStateSnapshot`。
- `GameController`（不依赖 UI）：`startGame/applyHumanMove/runAIMove/cancelAI`，内部持有 `Game` + `AIPlayer`。

> 注意：`GameController` 被编译进 `ai_core` 并被 `ai_core_tests` 测试，但 **GUI（MainWindow）并未使用它**；MainWindow 直接用 `Game` + `AIPlayer`（借助 QtConcurrent）完成人机回合。见第 16 节。

## 7. UI Architecture

- `MainWindow`（QMainWindow）：菜单栏（文件/游戏/帮助）、中央区域 = `BoardWidget` + 右侧控制面板（当前玩家 Label、状态 Label、新游戏按钮、悔棋按钮）。
- `BoardWidget`（自定义 QWidget）：绘制木质棋盘、网格、星位、黑白渐变棋子、最后一步红色标记；鼠标点击换算为棋盘坐标发出 `positionClicked`。
- `GameModeDialog`：三种模式
  - 双人对战
  - 人机对战（可选择执黑/执白、AI 难度：简单/标准/困难）
  - 网络对战（创建房间=主机执黑 / 加入房间=客户端执白，主机地址 / 端口）

## 8. Actual Runtime UI

- 默认主窗口标题「五子棋 - Gomoku」，最小 700×600。
- 新游戏 → 弹 `GameModeDialog` → 根据配置启动。
- 人机模式：
  - 玩家执黑：人类先手；玩家执白：AI 先手（`QTimer::singleShot` 100ms 触发）。
  - AI 回合通过 `QtConcurrent::run` 后台计算 `aiPlayer.search(snapshot)`，`QFutureWatcher` 回到主线程后 `game.makeMove`，期间锁定棋盘、显示「AI 思考中…」、等待光标。
  - 悔棋：AI 模式连撤两步，网络模式不支持悔棋。
- 网络模式：`NetworkManager`（Qt TCP + JSON：HELLO/MOVE/RESET），主机等待加入、客户端连接，窗口标题变化，禁用悔棋，收到对手 MOVE 才落子。

## 9. UI Implementations and Duplicates

- 当前 GUI 实现**唯一**：MainWindow + BoardWidget + GameModeDialog，无重复/冗余 UI 类。
- 但仓库同时存在**另一个独立的图形程序** `index.html`（单文件网页版，功能：皮肤、特效、音效、上传图片、本地存储）。二者是**两套完全独立、互不引用的可运行产物**，README 仅描述 `index.html`。

## 10. Player Architecture

- `IPlayer` 接口：`getMove(GameStateSnapshot)` + `cancel()`。
- `HumanPlayer`：`setMove/getMove` 适配器（实际 GUI 由点击事件驱动，未真正经它取棋）。
- `AIPlayer`：持有 `AIConfig` + `SearchEngine` + `cancelFlag`；`search()` 返回 `SearchResult`。
- GUI 中玩家类型（Human/AI）由 `Game::setPlayerType` / `isAITurn` 决定，AI 结果经 `aiPlayer.search()` 产生。

## 11. AI Architecture

- `AIConfig`：maxDepth=4、timeLimitMs=2000、maxNodes=1,000,000、useTT/useIterativeDeepening/useMoveOrdering=true、ttSizeMB=64。
- `GameStateSnapshot`：AI 专用只读快照（Board 拷贝 + currentPlayer + status + moveCount + `isTerminal/terminalWinner`）。
- `MoveGenerator`：首手出中心；否则只生成已有棋子附近半径 2 的空位；候选为空时回退全部空位。
- `Evaluation`：基于棋型/连线的静态评估（五连 1e6、活四 1e5、冲四 1e4、活三 5e3、眠三 1e3、活二 5e2、眠二 1e2、单子 20）+ 中心位置奖励；`evaluate = scoreFor(己方) - scoreFor(对方)`。
- `ZobristHash`：固定种子 20260901 的 64 位表；`computeHash` / `pieceKey`。
- `TranspositionTable`：`unordered_map`，`store/probe/clear`，按深度/节点类型（Exact/LowerBound/UpperBound）处理。
- `SearchEngine`：`findBestMove` → 迭代加深（1..maxDepth）→ `searchAtDepth` → `negamax`（Alpha-Beta，根节点用 `-INF/INF`，内部用 `-beta/-alpha`），TT 命中剪枝、着法排序（TT 着法优先 + 中心/近子启发）、`shouldAbort`（cancel / maxNodes / timeLimit）。
- AI 核心不 include Qt，纯 C++17，符合「AI 核心不依赖 UI」的设计原则。

## 12. Test Architecture

- `tests/ai_core_tests.cpp`（594 行）独立可执行，不依赖 Qt Widgets；覆盖：
  - 合法落子 x100、立即获胜/防守 x4 方向、终局停止、深度 1/2/3 安全返回、重复调用 x100
  - Timeout、Cancel、Node Limit、Iterative Deepening
  - Zobrist（同局面同哈希/落子变化/悔棋恢复）
  - TranspositionTable（store/probe/clear）
  - TT+Move Ordering 回归、默认配置回归、满盘和棋终局
  - AIPlayer / HumanPlayer / GameController（HvH 胜局、HvA 交替、AI 先手）、Game 和棋检测
- docs 记录：`ai_core_tests.exe` 最终 **27/27 PASS**。
- `docs/HVH_SMOKE_TEST.ps1`、`docs/HVA_SMOKE_TEST.ps1`、`docs/UIA_DIAG.ps1`：UI 冒烟/诊断脚本（均为项目内脚本，未在本机验证）。

## 13. Resources

- 无 `resources/` 目录（构建无 qrc）。
- `docs/screenshot_main.png`（168KB，程序主界面截图）。
- `五子棋项目开题答辩 PPT.pptx`（约 219KB，开题答辩 PPT）。
- 音效/皮肤等资源只出现在网页版 `index.html` 中（内联 CSS/JS，无需外部文件）。

## 14. Documentation

- 顶层：`README.md`（只写 index.html）、`PROJECT_STATE.md`（AI 重建 COMPLETE）、`AI_CAPABILITY.md`、`LOCALAI_V1.5_CHANGES.md`、`PROJECT_RECOVERY_REPORT.md`。
- `docs/`：大量 `AI_*` 报告（Cleanup、Integration Audit/Diagnosis/Recovery、Migration Preflight、Performance、Rebuild Phase1 设计、Core Test Result、Final QA）+ 环境安装指南、软件设计文档、开题 PPT 说明 + 冒烟脚本 / 截图。
- 记录了大量项目演进（v1.0 → v1.5 → 清理旧 AI → AI 重建（Phase 1-5）→ 27/27 通过）。

## 15. Git History Overview

- 远程 `main`：**仅 1 个提交** `2de5d6b Upload Gomoku game`（一次性上传 65 文件，11483 行）。
- 本地 `D:\gomoku`：**仅 1 个提交** `0df97e7 chore: initialize gomoku project`，工作树 clean。
- 二者 history 不连通（commit 不同），但**跟踪文件几乎完全一致**。
- 无法从 git 历史推断多人协作过程；作者名 `ariennbkls666-cell` 出现在远程提交。

## 16. Potential Conflicts

1. **README 与真实工程不一致（最明显）**：`README.md` 只用一页介绍 `index.html` 网页版，**完全未提及 C++/Qt 桌面应用**（CMake 工程、AI、网络）。而 `PROJECT_STATE.md`、`docs/AI_FINAL_QA_REPORT.md` 与源码均以桌面应用为主。仓库同时存在"网页版"与"桌面版"两套独立产物，README 只说明了其中一套。
2. **文档 API 与代码不一致（陈旧）**：`AI_CAPABILITY.md`、`LOCALAI_V1.5_CHANGES.md` 描述的接口（`getBestMove`、`SearchStats`、`setMaxDepth`、`include/TranspositionTable.h` 位于根、`TranspositionTable tt(size)`）与当前代码（`getMove/search`、`SearchResult`、`config.maxDepth`、`include/ai/...`、`TranspositionTable` 无构造参数）**对不上**。
3. **历史文档描述的系统已不存在**：`PROJECT_RECOVERY_REPORT.md` 及部分 `docs/AI_*` 提到 `LocalAIPlayer`、`IAIPlayer`、`LocalAIPlayer.h.bak`、`audit_test.cpp`、`benchmark_*`、`scripts/`、`CMakeLists.txt.bak`、`fix.ps1` 等。这些文件**在当前远程仓库中不存在**；当前仓库只有干净的 `include/ai + src/ai` 新 AI。即：这些文档是"旧状态"的快照，当前代码已演进/清理。
4. **版本号混乱**：`CMakeLists.txt` 的 `project VERSION` 为 **1.0**，但 `PROJECT_STATE.md`/`docs` 称已完成 **v1.5 / AI 重建**。版本标签不统一。
5. **GameController 与 GUI 双轨**：`ai_core`（被 `Gomoku` 链接）编译了 `GameController`，且被 `ai_core_tests` 测试；但 `MainWindow` 没有使用 `GameController`，而是直接用 `Game + AIPlayer + QtConcurrent`。存在"控制器层与 UI 层各写一套"的轻微重复/认知负担。
6. **本地与远程文件差异**：本地 `D:\gomoku` 缺少远程中的 `index.html`、`include/NetworkManager.h`、`src/NetworkManager.cpp`（即网页版 + 网络对战模块）。其余文件完全一致。

## 17. Known Issues

- 评估函数为**简化版**（侧重棋型 + 位置分），无复杂模式库、无开局库（文档已自我注明"待扩展"）。
- `GameController` 未接入 GUI；`HumanPlayer` 在 GUI 中未实际生效（GUI 走点击事件）。
- 网络模式能力未在仓库内验证（依赖双端联机），且网络对战不支持悔棋。
- CMake 因环境 `g++ -E` 不可用而使用手动 moc，属于环境妥协，但文档称可复现构建。

## 18. Unknown / Requires Confirmation

- 仓库"主要交付物"到底是网页版还是桌面版？README 只讲网页版，但工程/测试/大部分文档都以桌面版为主。
- `PROJECT_STATE.md`（2026-09-01）声称 AI 重建 COMPLETE，但 `CMakeLists VERSION` 为 1.0、远程提交日期为 2026-09-06——两者是否完全对应当前提交，需人工确认。
- 历史文档中提到、但当前仓库已不存在的 `LocalAIPlayer` 等旧 AI 文件，是否还需要？它们只在历史报告里出现，当前源码无引用。
- 网络对战、网页版 `index.html` 的运行行为，均未在本机实测。

## 19. Recommended Preparation Before Development

1. 明确「主交付物」：网页版 or 桌面版？或二者都交付？这决定后续改动重心与 README 目标。
2. 优先更新 `README.md` 到真实工程现状（桌面版架构 + 建构建方式 + 三种模式 + AI；`index.html` 可单独说明为"附带的单文件网页版"）。
3. 统一版本号：`CMakeLists project VERSION`、`PROJECT_STATE`、文档中的 v1.5 说法三者对齐。
4. 决定 `GameController` 与 `MainWindow` 直连 `Game+AIPlayer` 的取舍：要么 GUI 改用 `GameController`，要么把 `GameController` 从 `Gomoku` 链接中去掉（仅保留给测试）。
5. 若做 AI 增强（更复杂评估、开局库、性能统计），先补强 `Evaluation` 并补充 `ai_core_tests` 用例。

## 20. Final Project Understanding

- 这是一个《软件设计》课程五子棋项目，既包含**单文件网页版**（`index.html`），也包含**完整的 C++/Qt 桌面应用**。
- 桌面版具备：双人对战、**人机对战（AI：Minimax+Alpha-Beta+迭代加深+置换表+Zobrist+着法排序+超时/取消/节点限制）**、网络对战。
- AI 核心为独立的纯 C++17 静态库 `ai_core`，配套 27 项独立测试，并有 Qt 异步调度接入 GUI。
- 仓库为一次性快照（单提交），本地 `D:\gomoku` 是它的子集（缺网页版与网络模块）。
- 主要质量问题集中在**文档与代码脱节**（README 只讲网页版、AI 能力文档 API 过时、历史恢复报告描述已删除的系统、版本号不统一），而非运行/构建层面。

---
*报告生成时间：2026-09-07*
*来源：远程仓库 main@2de5d6b（本地 main@0df97e7 仅作对比）*
