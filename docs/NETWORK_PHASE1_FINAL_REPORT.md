# 联机功能第一阶段完整化 —— 最终报告

日期：2026-09-09
分支：codex/network-rewrite（基于 origin/整合版本3.0，ahead 2）
基线 commit：cd9ed77
本阶段 commit：`feat: complete online multiplayer phase 1`

## 1. 完成内容

在「已可工作的联机基础」上做增量增强，不重写底层网络：

1. **投降**：新增 `RESIGN` 协议、`OnlineSession::resign()` 接口、`resigned(Piece, GameStatus)` 信号、UI“投降”按钮；Host/Client 均可投降，对方判胜并进入终局。
2. **终端强制结果**：`GameEngine::forceResult(GameStatus)` 只改对局状态，不触碰 AI。
3. **状态机**：`Restarting` 已实际用于重赛协商；`requestRematch` / `answerRematch` / `handleRematch` / `handleResign` 完整实现。
4. **重赛**：双方同意后由 Host `hostStartNewGame()` 发 `NEW_GAME`；一方拒绝则回 `GameOver` 且不重置棋盘。
5. **断线 / 心跳**：保留 `handlePeerGone`、`onHeartbeatTick`、`onConnectTimeout`，未破坏。
6. **UI**：网络模式下“投降”与“再来一局”按钮按状态启停。

## 2. 修改文件

| 文件 | 改动 |
|------|------|
| `include/GomokuCore.h` / `src/GomokuCore.cpp` | 新增 `forceResult(GameStatus)` |
| `include/NetProtocol.h` | 新增 `kTypeResign = "RESIGN"` |
| `include/OnlineSession.h` / `src/OnlineSession.cpp` | 新增 resign / rematch 状态机与心跳控制 |
| `src/MainWindow.h` / `src/MainWindow.cpp` | 新增投降按钮、onResign / onNetResigned，按状态启停 |
| `tests/online_session_tests.cpp` | 新增用例 M–Q |
| `README.md` | 网络段文案与 UI 对齐（创建房间/加入房间/主机地址/端口/投降） |

## 3. 协议变化

- 新增消息类型：`RESIGN`，载荷 `{ "resigner": <Piece int> }`。
- 其余消息（`JOIN / WELCOME / STATE / MOVE / REQ_MOVE / REJECT / GAME_OVER / NEW_GAME / REMATCH / BYE / PING / PONG`）保持不变。
- 全消息带 `type` + `version`；JSON 换行分帧。

## 4. 状态机

`Disconnected → Connecting → WaitingOpponent → Playing → GameOver → Restarting → OpponentDisconnected / Error`

- 投降：`Playing` 下 `resign()` / `handleResign()` → `forceResult` → `GameOver` → 停心跳。
- 重赛：`GameOver` 下 `requestRematch()` → `Restarting`；对方接受 → Host 发 `NEW_GAME` 回 `Playing`；对方拒绝 → 回 `GameOver`。

## 5. 异常处理

- 非法坐标 / 非法 JSON / 非法落子：校验并拒绝，不崩溃、不破坏棋盘。
- 非当前玩家、已结束游戏无法落子。
- 重复消息幂等（重复 REMATCH、重复投降）。
- 断线 / 心跳超时 / 连接超时：分别进入 `OpponentDisconnected` / `Disconnected`。

## 6. 测试结果（真实运行）

| 测试 | 结果 |
|------|------|
| `online_session_tests.exe` | TOTAL checks: 134 FAILS: 0（A–R）EXIT=0 |
| `gomoku_core_tests.exe` | PASS EXIT=0 |
| `Gomoku.exe --ai-smoke` | AI_SMOKE OK 2 手 · H7 EXIT=0 |

## 7. 已知限制

- **双机局域网（Two-PC LAN）未实测**：本机（localhost）双实例测试通过，但未在两台物理电脑间做真机联机测试。
- **GUI 视觉/音效回归未逐项重跑**：本阶段未触碰棋盘/棋子皮肤、音效、设置模块；MainWindow 仅在网络模式下新增投降按钮。
- 防火墙 / 端口策略需用户端放行 `12345`。

## 8. 后续建议

- 在真实 LAN 环境跑一次三栏验收（创建房间 → 加入房间 → 对局 → 投降/重赛 → 断线）。
- 可进一步做“对局中途主机断线后重新加入”的恢复支持。

---

================================
NETWORK PHASE 1 FINAL QA
================================

Build: PASS
Local PvP: PASS
AI: PASS
Host: PASS
Client: PASS
Move Synchronization: PASS
Win/Lose: PASS
Resign: PASS
Rematch: PASS
Disconnect: PASS
Timeout: PASS（代码路径 + Test R 连接被拒；未做真实 8s 等待）
Invalid Message Protection: PASS
State Synchronization: PASS
Two-PC LAN: NOT TESTED
Regression: PASS（构建 / AI / 核心回归；棋盘皮肤、棋子皮肤、音效、设置在 GUI 中未在本次逐一重跑）
Documentation: PASS

================================
Overall: PASS (Two-PC LAN 待真机实测)
================================