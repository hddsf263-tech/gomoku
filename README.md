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
- 网络对战使用主机/客户端 TCP + JSON，`HELLO` / `MOVE` / `RESET` 消息语义与原 C++ 网络模块一致
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

## 文件说明

- `src/MainWindow.cpp` / `src/MainWindow.h`：主窗口与模式控制
- `src/BoardWidget.cpp` / `src/BoardWidget.h`：棋盘绘制、皮肤与特效
- `src/SkinDialog.cpp` / `src/SkinDialog.h`：皮肤、特效与音效设置
- `src/SoundManager.cpp` / `src/SoundManager.h`：合成音效与自定义音频播放
- `src/GomokuCore.cpp` / `include/GomokuCore.h`：棋盘规则与 AI 搜索
- `src/NetworkManager.cpp` / `include/NetworkManager.h`：局域网 TCP 对局
- `CMakeLists.txt`：构建规则
