# 网络系统重写审计（NETWORK REWRITE AUDIT）

> 本文件记录旧联机系统（NetworkManager + Qt TCP/JSON + HELLO/MOVE/RESET 协议）在被删除前的完整审计结果。
> 审计目的：确认旧系统范围、依赖关系、已知缺陷，以及新系统如何替代。

## 1. 旧联机相关文件

| 文件 | 角色 |
| --- | --- |
| `include/NetworkManager.h` | 旧网络管理类声明（非 QObject，使用 std::function 回调） |
| `src/NetworkManager.cpp` | 旧网络管理类实现（Qt TCP + 换行分隔 JSON） |
| `src/MainWindow.h` / `src/MainWindow.cpp` | 旧网络 UI 入口、状态管理、协议发送/接收的全部接线 |
| `CMakeLists.txt` | 把 `src/NetworkManager.cpp` 列入 `add_executable`，链接 `Qt6::Network` |
| `README.md` | 描述“网络对战使用主机/客户端 TCP + JSON，HELLO / MOVE / RESET” |

## 2. 旧 NetworkManager 的职责

- 作为 Host：`server_->listen(QHostAddress::Any, port)`，接受第一个客户端；`attachSocket()` 接管连接。
- 作为 Client：`connectToHost(host, port)` 建立 TCP 连接。
- 发送/接收以 `\n` 分隔的 JSON 行，用 `buffer_` + `indexOf('\n')` 做帧化。
- 消息类型：`HELLO`（携带 color）、`MOVE`（携带 row/col）、`RESET`。
- 通过 `std::function` 回调（`onConnected` / `onDisconnected` / `onError` / `onMove` / `onHello` / `onReset`）通知 `MainWindow`。
- **不做任何棋局校验、不做状态同步、不做断线重连**——纯传输层。

## 3. MainWindow 中依赖旧网络的代码

- 头文件：`#include "NetworkManager.h"`；成员 `NetworkManager network_;`
- 构造器：设置 `onConnected/onDisconnected/onError/onMove/onHello/onReset` 六类回调。
- `onConnected`：Host 置 `myColor_=Black` 并发 `sendHello(0)`；Client 置 `myColor_=White` 并发 `sendHello(1)`；随后 `restartCurrent(false)`。
- `onHello`：`myColor_ = color==0 ? White : Black`（对端颜色取反）。
- `onMove`：仅当 `netConnected_ && 对局中 && currentPlayer()!=myColor_` 时 `doPlace(..., remote=true)`。
- `onReset`：`restartCurrent(false)`。
- `onDisconnected`：`netConnected_=false`。
- `startNetwork()` / `stopNetwork()` / `onNetAction()`：开始/断开 Host 或 Client。
- `setMode()`：离开 Network 模式时 `network_.disconnectPeer()`。
- `doPlace()`：本地落子后若处于 Network 模式则 `network_.sendMove(row,col)`。
- `restartCurrent(bool notifyRemote)`：`notifyRemote && netConnected_` 时 `network_.sendReset()`。
- `canHumanInput()`：Network 模式下要求 `netConnected_ && currentPlayer()==myColor_`。
- `updateStatus()`：大量使用 `network_.role()`、`myColor_`、`netConnected_`。

## 4. CMake 中的网络依赖

- `find_package(Qt6 ... Network ...)`
- `add_executable(Gomoku ... src/NetworkManager.cpp ...)`
- `target_link_libraries(Gomoku ... Qt6::Network ...)`

## 5. UI 中进入旧网络的入口

- 侧边栏模式按钮 `modeNet_`（“网络对战”）→ `setMode(Mode::Network)`。
- `netOptions_` 面板：`netHost_`（创建房间）、`netClient_`（加入房间）、`netAddress_`（主机地址，默认 127.0.0.1）、`netPort_`（端口，默认 12345）、`netAction_`（创建/加入）、`netStatus_`（状态提示）。

## 6. 旧网络协议

```
Client -> Host : { "type":"HELLO", "color":1 }   （客户端手握手）
Host   -> Client: { "type":"HELLO", "color":0 }
Host   -> Client: { "type":"MOVE", "row":r, "col":c }
Host   -> Client: { "type":"RESET" }
Client -> Host : { "type":"MOVE", "row":r, "col":c }
```
- 帧格式：每条消息一行 JSON（Compact 后 `\n` 结尾）。
- 无握手序号、无状态快照、无心跳、无错误码。

## 7. 已知问题

1. **颜色/回合竞态**：Host 与 Client 各自发送 HELLO，又各自通过 `onHello` 取“对端颜色”，逻辑冗余且存在竞态窗口。
2. **无全量状态同步**：只有 HELLO+MOVE，没有 BOARD_STATE 快照；中途加入或发生失步后无法自动恢复。
3. **Host 非权威裁判**：Client 落子仅由 `MainWindow::onMove` 的“是不是我的回合”+`doPlace` 的 `canPlace` 兜底；NetworkManager 本身零校验。
4. **非法落子静默丢弃**：非法/重复落子被 `canPlace` 挡掉后不通知对端、不回发状态，可能造成双方视图不一致。
5. **逻辑散落在 MainWindow**：`netConnected_`、`myColor_`、按钮/标签状态、回调全部在 UI 层，网络层与 UI 强耦合。
6. **无断线重连/超时**：没有心跳、没有空闲超时，长期无操作时连接状态不可靠。
7. **类名与职责不清**：`NetworkManager` 只做传输，却承担“管理”帧名，职责边界混乱。

## 8. 必须删除的代码

- `include/NetworkManager.h`
- `src/NetworkManager.cpp`
- `MainWindow` 中对 `NetworkManager` / `network_` 的全部引用、回调、`netConnected_` 逻辑。
- `README.md` 中“TCP + JSON / HELLO / MOVE / RESET”的旧描述。
- `CMakeLists.txt` 中的 `src/NetworkManager.cpp`。
- 旧 `MOVE`/`RESET`/`HELLO` 协议语义。

## 9. 必须保留的代码

- 棋盘、皮肤、棋子皮肤、落子/获胜特效、音效、音效开关（`BoardWidget` / `SkinDialog` / `SoundManager`）。
- 双人本地对战、人机对战、AI 难度、悔棋、重新开始、胜负/平局判断、设置持久化（`GameEngine` / `GomokuCore` / AI 核心）。
- **禁止修改 `GomokuCore` 的 AI 搜索代码。**
- 测试 `tests/core_tests.cpp`（纯核心测试，不触碰网络）。

## 10. 新系统如何替代旧系统

- 新网络分层：**传输层**（`net::NetLink`，QObject，负责 TCP 监听/连接、`\n` JSON 帧化、信号事件）+ **会话层**（`OnlineSession`，QObject，负责对局协调、Host 权威裁判、状态同步、悔棋/重开、断线处理）。
- `OnlineSession` 注入一个 `GameEngine&`（即 MainWindow 的 `game_`），网络层与棋局核心解耦，核心零改动。
- Host 为权威裁判：本地/对端落子都经 Host 校验，合法才落地并广播；非法发送 `REJECT` + `STATE` 全量重同步。
- 连接后 Host 下发 `WELCOME`（分配颜色）+ `STATE`（全量状态），保证双方一致。
- 通过信号驱动 UI（`connected` / `disconnected` / `moveCommitted` / `moveRejected` / `boardReset` / `gameOver` / `opponentLeft` / `errorOccurred`），全程事件驱动、无阻塞、无无限循环。
- 新协议：`JOIN / WELCOME / STATE / MOVE / REQ_MOVE / REJECT / REMATCH / REQ_REMATCH / GAME_OVER / BYE`。
