# 五子棋联机系统完全重写 —— 最终报告

> 文档：docs/NETWORK_REWRITE_FINAL_REPORT.md
> 项目路径：`D:\gomoku`
> 分支：`codex/network-rewrite`（基于 `origin/整合版本3.0`）

---

## 1. 重写原因

原项目使用旧的 `NetworkManager` + TCP + JSON（`HELLO` / `MOVE` / `RESET`）实现联机对战，但长期无法稳定工作。

本次任务要求：

> **删除旧联机实现 → 清理旧依赖 → 重新设计 → 重新实现 → 完整测试 → 最终实现两台不同电脑稳定对局。**

禁止继续在旧底层上打补丁。因此本次彻底删除旧的 `NetworkManager`，并全新实现 `NetLink` + `NetProtocol` + `OnlineSession` 三层结构。

---

## 2. 删除了哪些旧代码

- 删除 `include/NetworkManager.h`
- 删除 `src/NetworkManager.cpp`
- 清理旧 `HELLO` / `MOVE` / `RESET` 协议语义
- 清理 `MainWindow` 中所有对旧网络类的依赖、状态变量与信号槽
- 移除 `CMakeLists.txt` 中对旧网络模块的引用
- 清理了早期遗留、未被构建引用的旧联机文件：
  - `include/NetworkClient.h`、`include/NetworkProtocol.h`、`include/NetworkServer.h`
  - `src/NetworkClient.cpp`、`src/NetworkServer.cpp`

（`SoundManager.cpp` 中的 `QDataStream` 用于音频合成，与联机无关，予以保留。）

---

## 3. 新架构

```
MainWindow (UI)
      │
      ▼
OnlineSession (会话状态机 / Host 权威裁判)
      │  (依赖 GomokuCore GameEngine）
      ▼
NetLink (TCP 传输层：监听/连接/帧化/信号)
      │
      ▼
NetProtocol (协议层：消息类型 + JSON 换行分隔帧)
```

- `NetProtocol`：纯“表示”层，不依赖棋局核心，可独立测试。
- `NetLink`：纯“传输”层，不接触棋局核心。
- `OnlineSession`：协调对局，Host 权威裁判、回合校验、状态广播、胜负判定、重赛、断线；事件驱动，不阻塞 UI。

---

## 4. 网络协议

消息类型：

| 类型 | 方向 | 含义 |
|---|---|---|
| `JOIN` | Client → Host | 请求加入 |
| `WELCOME` | Host → Client | 分配颜色（客户端为白方，后手） |
| `STATE` | Host → Client | 整局棋盘状态同步 |
| `MOVE` | Host → Client | 广播已判定合法的落子 |
| `REQ_MOVE` | Client → Host | 客户端请求落子 |
| `REJECT` | Host → Client | 拒绝落子（附原因） |
| `GAME_OVER` | Host → Client | 终局判定 |
| `NEW_GAME` | Host → Client | 重赛开局 |
| `REMATCH` | 双方 | 重赛请求/应答（`accept`） |
| `BYE` | 双方 | 主动离开 |
| `PING` / `PONG` | 双方 | 心跳 |

帧化：紧凑 JSON + `\n` 行分隔，天然解决 TCP 半包/粘包/拆包。消息统一带 `type` + `version`。

默认端口：`12345`。

---

## 5. Host 工作方式

- `startHost(port)`：监听 TCP 端口。
- 客户端加入后：发送 `WELCOME(White)` + `STATE`，主机扮演黑方并先手。
- 本端落子：`localMove` 直接 `hostCommitMove`。
- 收到客户端 `REQ_MOVE`：校验（位置合法、轮到该方、未占用、对局未结束），合法则 `hostCommitMove`，不合法则回 `REJECT`。
- `hostBroadcastState`：同步整局 `moves` 直到当前局面。
- 终局：`checkGameOverAfterMove` 判定胜负/平局，发 `GAME_OVER`。

## 6. Client 工作方式

- `connectToHost(host, port)`：发起 TCP 连接。
- 连接成功后发送 `JOIN`。
- 收到 `WELCOME`：确定颜色（白方，后手）。
- 收到 `STATE`：回放整局棋盘。
- 本端落子：`localMove` 发送 `REQ_MOVE`，等待 Host 回 `MOVE` 后才落地。
- 收到 `MOVE`：落地并刷新；若触发终局则本端也进入 `GameOver`。

---

## 7. 断线处理

- 心跳：每 3 秒发送 `PING`，收到 `PONG` 更新活跃时间；超过 12 秒无活动判定超时。
- 对端断开 / 心跳超时 / 收到 `BYE`：进入 `OpponentDisconnected`，通知 UI 并停止心跳。
- 客户端连接超时：8 秒未建立连接则提示并回到 `Disconnected`。
- 网络异常（socket 错误）触发 `errorOccurred` 并进入对应状态，不进入死循环、不阻塞 UI。

---

## 8. 测试结果

| 项目 | 结果 | 说明 |
|---|---|---|
| Build（CMake configure + ninja） | **PASS** | 主程序与所有测试目标均编译/链接成功 |
| 单元测试 `gomoku_core_tests` | **PASS** | 棋盘规则单元测试全部通过（`gomoku core ok: ai-win-move, win detect, undo`，EXIT=0） |
| 单元测试 `online_session_tests` | **PASS** | 联机协议与会话测试全部通过（**TOTAL checks: 92, FAILS: 0, EXIT=0**） |
| Localhost（单机本机回环） | **PASS** | `online_session_tests` 在单进程内通过 TCP 回环完成 Host+Client 的完整对局、非法落子拒绝、胜负判定、重赛、断线等 **A–L 共 12 组场景** |
| LAN（局域网） | **NOT TESTED** | 需要第二台真实电脑 |
| Two-machine（双机） | **NOT TESTED** | 需要第二台真实电脑 |
| AI | **PASS** | `Gomoku.exe --ai-smoke` 输出 `AI_SMOKE OK 2 手 · H7`，EXIT=0 |
| Local PvP | **PASS** | 由 `gomoku_core_tests` 覆盖胜负/平局/规则；本地双方逻辑未改动 |
| Skin | **PASS** | 代码未改动，构建通过（界面视觉未人工核验，见“已知限制”） |
| Sound | **PASS** | 代码未改动，构建通过（界面听觉未人工核验，见“已知限制”） |
| Disconnect | **PASS** | `online_session_tests` 场景 E（客户端离开）、J（主机关闭）、L（终局后断线） |
| Rematch | **PASS** | `online_session_tests` 场景 D（同意重赛重置）、K（一方拒绝重赛，棋盘未重置） |

### online_session_tests 覆盖场景（A–L）

- A：`NetProtocol` 帧化/解析往返 + 非法 JSON 拒绝。
- B：Host+Client 单线程连接，主机黑 `(7,7)`、客户端白 `(8,8)`，校验双方棋局同步。
- C：客户端在已占用格 `(7,7)` 请求落子，Host 回 `REJECT`，Host 不落地。
- D：黑方（主机）连五获胜，双方到 `GameOver`，随后重赛双方重置。
- E：客户端 `leaveSession()`，主机收到 `opponentDisconnected`。
- F：白方发出越界请求 `(99,99)`，Host 回 `REJECT`（原因 `INVALID_POSITION`），Host 棋局未变化。
- G：当前轮到黑方时白方本地 `localMove` 被拒绝（不发送请求），双方棋盘均未变化。
- H：游戏结束后客户端 `localMove` 返回 false，不能再落子。
- I：纵向五连获胜，Host 与 Client 均判定 `BlackWin`，双方进入 `GameOver`。
- J：主机主动关闭，客户端感知 `opponentDisconnected`。
- K：一方拒绝重赛（`REMATCH accept=false`），请求方 Host 收到 `rematchDeclined`，棋盘未重置。
- L：终局后一方断线，另一方收到 `opponentDisconnected`。

> 重要：以上 **Localhost** 结果是真实的（由同一进程内的 TCP 回环测试产生，本轮实际运行通过：92 checks / 0 FAILS / EXIT=0）。**LAN / 双机测试并未执行**，因为本机仅为单台电脑。切勿将 Localhost 结果表述为“两台电脑测试通过”。

## 9. 构建结果

```text
CMake configure : PASS
Ninja build     : PASS
生成产物        : Gomoku.exe, gomoku_core_tests.exe, online_session_tests.exe
```

使用的工具链：

```text
CMake  : D:\Qt\Tools\CMake_64\bin\cmake.exe
Ninja  : D:\Qt\Tools\Ninja\ninja.exe
编译器 : D:\Qt\Tools\mingw1310_64\bin\g++.exe (GNU 13.1.0)
Qt     : D:/Qt/6.11.2/mingw_64
```

---

## 10. 运行结果

```text
Gomoku.exe --ai-smoke ：
  AI_SMOKE state checked=1 status=你的回合 board=1 rect=790x632
  AI_SMOKE OK 2 手 · H7
  EXIT=0

Gomoku.exe --screenshot shot_net.png ：EXIT=0（截图文件已生成）
```

主程序可正常启动并完成 AI 冒烟自检。

---

## 11. 已知限制

- **双机测试未执行**：本机只有一台电脑，无法完成两台不同电脑的真实对局验证。
- **界面视觉 / 音效未人工核验**：由于沙箱无法进行可视化交互，皮肤、音效等仅确认“代码未改动 + 构建通过 + 冒烟测试通过”，未做人工肉眼/听觉验收。
- **只允许本地监听单一对端**：`NetLink` 目前只服务一个客户端；若需多人或排队，需要扩展。
- **天然回环 vs 真实局域网差异**：Localhost 通过了进程内 TCP 回环测试，但真实局域网可能受防火墙、路由、NAT、网络策略影响，需在真实环境验证。

---

## 12. 两台电脑实际测试结果

**尚未执行**（当前环境仅一台电脑）。

完成本项需要两台真实 Windows 电脑，并通过局域网连接，按以下步骤验证：

1. 电脑 A：网络对战 → 创建主机，默认端口 `12345`，记录本机 IPv4。
2. 电脑 B：网络对战 → 加入主机，输入电脑 A 的 IPv4 与端口 `12345`。
3. 依次验证：连接成功、颜色分配正确、回合正确、落子实时同步、胜负/平局同步、重赛、断线提示。
4. 若连接失败，检查 Windows 防火墙放行 TCP `12345`。

> 该结果如实标注为 `NOT TESTED`，不作任何伪造。

---

## 13. 后续升级建议

- 增加“可重连 / 断线重连”能力（当前断线后需重新创建或加入）。
- 支持多对局会话与用户昵称。
- 引入 UDP 发现（局域网自动发现主机，免手动输 IP）。
- 增加落子编号/时间戳，便于对局回放与校验。
- 为 `OnlineSession` 增加更细的 `Restarting` 状态与超时协商。
