# 五子棋 Gomoku（Qt / C++ 版）

将此前网页版的功能用 C++ / Qt 重新实现，界面布局与交互沿用网页版设计，并直接取代仓库中原本上传的 JavaScript 页面。

## 功能

- 15 x 15 标准棋盘，黑方先手，点击交叉点落子
- 自动判定五连胜负，平局自动识别
- 悔棋、重新开始、当前手数与坐标提示
- 棋盘皮肤：翡翠、原木、墨玉、红木，支持上传照片作为棋盘背景
- 棋子皮肤：经典、玉石、曜石、琥珀，支持上传照片作为黑棋/白棋背景
- 落子特效：扩散光环、星光粒子、关闭
- 获胜特效：金色脉冲、彩带爆裂、关闭
- 落子音效：木声、清脆、水滴、静音，支持导入自定义音频
- 获胜音效：和弦、欢快、科幻、静音，支持导入自定义音频
- 全局音效开关
- 双人对战、人机对战、网络对战三种模式
- AI 难度：简单、标准、困难
- 网络对战：全新 TCP 主机/客户端实现，主机为权威裁判，实时同步棋局与胜负
- 设置通过 `QSettings` 持久化

## 构建

要求 Qt 6（Widgets、Network、Concurrent、Multimedia）与 CMake。

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt安装目录>
cmake --build build
```

运行：

```bash
./build/Gomoku
```

## 网络对战

网络对战由全新的三层模块实现：

- `NetProtocol`：协议层，定义消息类型（`JOIN` / `WELCOME` / `STATE` / `MOVE` / `REQ_MOVE` / `REJECT` / `GAME_OVER` / `NEW_GAME` / `REMATCH` / `BYE` / `RESIGN` / `PING` / `PONG`），JSON 换行分隔帧（天然解决 TCP 半包/粘包）。
- `NetLink`：传输层，封装 TCP 监听/连接、帧化与事件信号，不接触棋局核心。
- `OnlineSession`：会话状态机，Host 权威裁判、回合校验、状态广播、胜负判定、重赛与断线处理；事件驱动，不阻塞 UI。

### 创建房间（主机 / Host）

1. 在主界面选择“网络对战”模式，然后在侧边栏选择“创建房间”（主机，Host）。
2. 在侧边栏的“端口”设置端口（默认 `12345`），点击“创建房间”。
3. 程序会显示本机局域网 IP 与监听端口，等待对手加入。
4. 主机扮演黑方并先手。

### 加入房间（客户端 / Client）

1. 在另一台电脑上选择“网络对战”模式，然后在侧边栏选择“加入房间”（客户端，Client）。
2. 在“主机地址”填写主机的局域网 IP，并设置相同的“端口”（默认 `12345`）。
3. 点击“加入房间”，等待主机确认；连接成功后由主机分配颜色（客户端为白方，后手）。

### 默认端口

默认端口为 `12345`。

### 查看本机 IP

- 在主机启动后会直接显示本机局域网 IPv4 地址。
- 也可以在命令行执行 `ipconfig`，在“无线局域网适配器 / 以太网适配器”中查看 IPv4 地址。

### Windows 防火墙

如果双方在同一局域网内仍无法连接，通常是 Windows 防火墙拦截了端口：

1. 打开“控制面板 → Windows Defender 防火墙 → 高级设置”。
2. 在“入站规则”中新建规则，选择“端口”，TCP，输入 `12345`，允许连接。
3. 或临时允许 `Gomoku.exe` 通过防火墙（或在启动时弹出的防火墙提示中勾选“允许访问”）。

### 对局规则

- Host 为权威裁判，校验每步落子是否合法、是否轮到该方；非法落子会被拒绝（客户端提示原因）。
- 落子、胜负、平局、重赛均通过协议实时同步。
- 断线检测：通过心跳 `PING` / `PONG` 与超时机制，一方的异常断开会通知另一方并结束会话。
- 投降：对局进行中任一方可点击“投降”，对方判胜并进入终局，随后仍可发起重赛。
- 重赛：一局结束后，任一方点击“再来一局”，对方确认后双方清空棋盘重新开局。

## 文件说明

- `src/MainWindow.cpp` / `src/MainWindow.h`：主窗口与模式控制
- `src/BoardWidget.cpp` / `src/BoardWidget.h`：棋盘绘制、皮肤与特效
- `src/SkinDialog.cpp` / `src/SkinDialog.h`：皮肤、特效与音效设置
- `src/SoundManager.cpp` / `src/SoundManager.h`：合成音效与自定义音频播放
- `src/GomokuCore.cpp` / `include/GomokuCore.h`：棋盘规则与 AI 搜索
- `include/NetProtocol.h`：联机协议（消息类型 + JSON 帧化/解析，纯表示层，可独立测试）
- `include/NetLink.h` / `src/NetLink.cpp`：TCP 传输层（监听/连接/帧化/信号）
- `include/OnlineSession.h` / `src/OnlineSession.cpp`：联机会话状态机（Host 权威裁判）
- `tests/core_tests.cpp`：棋盘核心单元测试
- `tests/online_session_tests.cpp`：联机协议与会话单元测试
- `CMakeLists.txt`：构建规则