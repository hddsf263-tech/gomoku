# 五子棋 Gomoku

一个使用 C++ 与 Qt 开发的桌面版五子棋，支持本地双人、人机对战与双设备网络对战，并内置 AI 博弈核心。

## 功能

### 对弈
- 15 × 15 标准棋盘，黑方先手，点击交叉点落子
- 自动判定五连胜负，平局自动识别
- 游戏模式：本地双人、人机对战（简单 / 普通 / 困难）、网络对战（主机 / 客户端）
- 悔棋、重新开始、当前手数与最后一步坐标提示（例如 `12 手 · C8`）

### 外观与音效
- 棋盘皮肤：翡翠、原木、墨玉、红木，支持上传照片作为棋盘背景
- 棋子皮肤：经典、玉石、曜石、琥珀，支持上传照片作为黑棋 / 白棋背景
- 落子特效：扩散光环、星光粒子、关闭
- 获胜特效：金色脉冲、彩带爆裂、关闭
- 落子音效：木声、清脆、水滴、静音
- 获胜音效：和弦、欢快、科幻、静音
- 支持导入本地音频文件作为自定义落子 / 获胜音效
- 全局音效开关，皮肤、特效与音效设置记忆保留

## 构建

需要 CMake（≥ 3.16）与 Qt（Qt 6 或 Qt 5.15，需含 Core、Gui、Widgets、Concurrent、Network、Multimedia 模块）。

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<Qt 安装路径>
cmake --build build --config Release
```

生成的桌面程序与测试程序位于 `build` 目录：
- `build/Gomoku.exe`：主程序
- `build/ai_core_tests.exe`：AI 核心单元测试

实际构建时可参考下面的 MinGW 工具链（以 Qt 6.11.2 为例）：

```powershell
$env:Path = "C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.2\mingw_64\bin;$env:Path"
```

发布前可用 `windeployqt` 收集 Qt 运行库。

## 目录说明

- `src`：Qt 界面与核心业务源码（`MainWindow`、`BoardWidget`、`SettingsDialog`、`SoundPlayer`、`AppSettings` 等）
- `include`：棋盘、游戏、网络与 AI 接口头文件
- `src/ai`：AI 博弈核心
- `tests`：AI 核心单元测试
- `index.html`：旧版单文件网页演示，仅作参考保留，已由 Qt 桌面应用取代
- `docs`：课程设计相关文档

## 文件说明

- `README.md`：项目说明
- `index.html`：旧版网页版五子棋演示（无外部依赖，双击可玩）
