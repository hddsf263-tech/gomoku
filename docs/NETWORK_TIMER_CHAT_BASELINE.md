# 联机计时器 + 聊天 —— 基线审计

日期：2026-09-09
分支：`整合版本5.0`
当前 commit：`365c6b5`（chore: ignore 整合版本5.0 packaging folder）

## 当前状态

| 项目 | 结果 |
|------|------|
| Build | PASS（Ninja / Qt 6.11.2 / MinGW 13.1.0） |
| Host | PASS（`startHost`, 端口监听, 等对手加入） |
| Client | PASS（`connectToHost`, JOIN → WELCOME → STATE） |
| 基础落子同步 | PASS（REQ_MOVE → 校验 → MOVE 广播） |
| 胜负同步 | PASS（MOVE 落地 + GAME_OVER 广播, 双方状态一致） |
| 网格核心回归 | PASS（`gomoku_core_tests` EXIT=0） |
| 联机会话回归 | PASS（`online_session_tests` 134 checks, 0 fail, A–R） |

## 当前网络模块

- `include/NetProtocol.h`：协议表示层。全消息 `type`+`version`，JSON 紧凑序列化 + `\n` 换行分帧。
- `include/NetLink.h` / `src/NetLink.cpp`：TCP 传输层。Host 监听 / Client 连接；`QByteArray buffer_` 按换行切帧，解决半包/粘包；信号 `connected / disconnected / errorOccurred / messageReceived / peerConnected / peerDisconnected`。
- `include/OnlineSession.h` / `src/OnlineSession.cpp`：会话状态机。Host 权威裁判、回合校验、状态广播、胜负判定、重赛、断线、心跳。

## 当前网络协议消息

`JOIN / WELCOME / STATE / MOVE / REQ_MOVE / REJECT / GAME_OVER / NEW_GAME / REMATCH / RESIGN / BYE / PING / PONG`

## 当前联机 UI（MainWindow 侧边栏）

- 模式：`创建房间` / `加入房间`（`netHost_` / `netClient_`）
- 参数：`主机地址`（`netAddress_`）、`端口`（`netPort_`，默认 12345）
- 动作：`netAction_`（创建房间 / 加入房间 / 断开连接）
- 状态：`netStatus_`
- 对局内按钮：`悔棋` / `再来一局` / `投降`

## 本次目标

在**不重写**以上联机系统的前提下，做增量扩展：

1. 联机对局计时器：双方独立棋钟，Host 权威计时，随回合切换，超时判负，新局重置，断线/终局停止。
2. 联机聊天：复用现有 TCP 连接，新增 `CHAT_MESSAGE`，Host 校验并转发，实时显示，与棋局/计时解耦。
