# 联机功能第一阶段完整化 —— 基线审计（已更新为完成态）

日期：2026-09-09
分支：codex/network-rewrite（基于 origin/整合版本3.0，ahead 2）
基线 commit：cd9ed77 test: expand online session coverage A-L and finalize rewrite report
当前状态：本阶段增强功能已实现并通过回归测试（见 docs/NETWORK_PHASE1_FINAL_REPORT.md）

## 0. 说明

本审计基于「在当前已可工作的联机基础之上做增量增强，不重写底层网络」的原则。
当前联机系统为三层架构：

- 协议层 `include/NetProtocol.h`：JSON 换行分帧，独立于棋局核心。
- 传输层 `include/NetLink.h` + `src/NetLink.cpp`：TCP 监听/连接，事件驱动，无阻塞循环。
- 会话层 `include/OnlineSession.h` + `src/OnlineSession.cpp`：Host 权威裁判、状态同步、重赛、断线、投降。

## 1. 当前已正常工作的功能（基线验证）

构建：PASS（build_net_rewrite，CMake + Ninja + mingw1310_64 + Qt 6.11.2）
单元测试：PASS
- online_session_tests：TOTAL checks 131, FAILS 0（覆盖协议、连接同步、非法落子、连五/纵线胜负、断线、重赛、投降等 A–Q）
- gomoku_core_tests：PASS
- Gomoku `--ai-smoke`：PASS

功能逐项：

| 功能 | 状态 | 说明 |
|------|------|------|
| Host 创建 (listen) | PASS | startHost(port)，listen(0) 可查询分配端口 |
| Client 连接 | PASS | connectToHost(host, port)，超时 8s |
| 双方身份 | PASS | Host=Black，Client=White，WELCOME 分配 |
| 棋盘同步 | PASS | STATE 全量 + MOVE 增量 |
| 轮流落子 | PASS | currentPlayer 校验 |
| 胜负同步 | PASS | GAME_OVER + winner 广播 |
| 非法落子保护 | PASS | REJECT（INVALID_POSITION / NOT_YOUR_TURN / GAME_OVER / CELL_OCCUPIED） |
| 重赛协商 | PASS | REMATCH accept=true/false，双方同意后重置 |
| 重赛拒绝 | PASS | 一方拒绝不重置 |
| 断线处理 | PASS | opponentDisconnected 信号，OpponentDisconnected 状态 |
| 心跳/超时 | PASS | PING/PONG，3s 间隔，12s 超时 |
| 连接超时 | PASS | 8s 超时 -> Disconnected |
| 终局后禁用落子 | PASS | GameOver 状态 localMove 返回 false |
| 旧局消息污染 | PASS | 协议带 version，重赛通过 reset 清空 |

## 2. 本阶段已完成增强功能（原缺失项）

| 功能 | 状态 | 说明 |
|------|------|------|
| 投降 (Resign) | PASS | 协议 `RESIGN` + 会话接口 `resign()` + UI 投降按钮；Host/Client 均可投降，对方判胜 |
| 投降感知 | PASS | `resigned(Piece, GameStatus)` 信号，双方状态同步至 GameOver 并停止心跳 |
| 状态机完整性 | PASS | `Restarting` 已实际用于重赛协商；requestRematch/answerRematch/declined 均可正确转换 |
| UI 状态提示 | PASS | 网络模式下“投降”按钮按状态启停；“再来一局”按钮启停；断线/终局文案 |
| 投降后重赛 | PASS | 投降终局后仍可发起重赛并成功开局 |
| 非法 JSON 保护 | PASS | NetProtocol::parse 对非法 JSON 返回 false，不崩溃 |
| 重复消息幂等 | PASS | 重复 REMATCH / 投降保持幂等，不破坏状态 |

## 3. 架构风险

- 无阻塞循环：确认无 while(true)。
- Host 权威：确认 hostValidateMove + hostCommitMove。
- 事件驱动：确认全部通过信号/槽。
- NetLink 的 handleDisconnect 在 Host 身份下对 wasConnected 判断合理。
- RESIGN 的 resigner 解析：Host 收到 Client RESIGN 时 fallback=White，Client 收到 Host 广播时 fallback=Black，实际以消息内显式字段为准。

## 4. 本阶段修改文件

- include/GomokuCore.h、src/GomokuCore.cpp：新增 `forceResult(GameStatus)`，只改状态、不触碰 AI。
- include/NetProtocol.h：新增 `kTypeResign = "RESIGN"`。
- include/OnlineSession.h、src/OnlineSession.cpp：新增 resign/answerRematch/requestRematch 完整状态机与心跳。
- src/MainWindow.h、src/MainWindow.cpp：新增投降按钮、onResign/onNetResigned，并按状态启停。
- tests/online_session_tests.cpp：新增用例 M–Q（Host/Client 投降、投降后重赛、重赛状态机、幂等）。

## 5. 结论

联机第一阶段功能已实现并通过回归测试；局域网双机实测尚未进行，见最终报告的 `Two-PC LAN: NOT TESTED`。