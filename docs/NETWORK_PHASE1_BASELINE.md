# 联机功能第一阶段完整化 —— 基线快照

日期：2026-09-09
分支：codex/network-rewrite（基于 origin/整合版本3.0，ahead 2）
基线 commit：cd9ed77 test: expand online session coverage A-L and finalize rewrite report

## 1. 基线构建状态

构建：PASS（build_net_rewrite，CMake + Ninja + mingw1310_64 + Qt 6.11.2）
单元测试：PASS
- online_session_tests：TOTAL checks 92, FAILS 0（A–L）
- gomoku_core_tests：PASS

## 2. 基线联机状态（增强前）

- 三层架构：NetProtocol（协议）/ NetLink（TCP 传输）/ OnlineSession（会话状态机）。
- Host 权威裁判：hostValidateMove + hostCommitMove。
- 客户端镜像：clientApplyWelcome / clientApplyState / clientApplyMove。
- 已有消息类型：JOIN / WELCOME / STATE / MOVE / REQ_MOVE / REJECT / GAME_OVER / NEW_GAME / REMATCH / BYE / PING / PONG。
- 状态机：Disconnected / Connecting / WaitingOpponent / Playing / GameOver / Restarting / OpponentDisconnected / Error。其中 Restarting 枚举存在但未实际使用。
- 心跳：PING/PONG，3s 间隔，12s 超时；连接超时 8s。
- 默认端口：12345。

## 3. 基线已知问题 / 缺失（本阶段要补）

- 无投降（协议/接口/UI 均缺）。
- Restarting 状态未真正用于重赛协商。
- 投降后无法重赛。
- UI 对重赛进行中 / 投降无专门文案。
- 缺少投降、重赛状态机、幂等性的测试覆盖。

## 4. 基线用户流程

- 进入主界面 → 侧边栏选“网络对战”。
- Host：选“创建房间”，填端口，点“创建房间”，显示本机 IP + 端口，等待加入。
- Client：选“加入房间”，填“主机地址”+“端口”，点“加入房间”，连接后由主机分配颜色。
- 对局中由 Host 校验落子并广播；终局广播 GAME_OVER。

## 5. 本阶段不重写底层网络

底层 NetLink / NetProtocol / OnlineSession 连接链路保持基线版本，仅在会话层与 UI 层做增量增强。