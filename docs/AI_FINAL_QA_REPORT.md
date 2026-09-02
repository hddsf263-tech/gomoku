# AI 人机对弈系统重建 - 最终 QA 报告

**日期**: 2026-09-01
**阶段**: Phase 5 Final QA
**工作目录**: `D:\gomoku`

## 验收结果

| 验收项 | 结果 | 证据 |
| --- | --- | --- |
| Build | PASS | `Gomoku` 与 `ai_core_tests` 编译成功 |
| Runtime | PASS | UI 自动化启动并找到主窗口 |
| Human vs Human | PASS | 冒烟测试：交替落子、黑方五连、重开、重复落子拦截 |
| Human vs AI | PASS | 冒烟测试：玩家先手、AI 先手 |
| Win | PASS | AI 立即获胜 x4 方向；HvH 黑方获胜；GameController HvH 胜局 |
| Lose | PASS | 对手五连终局立即停止搜索 |
| Draw | PASS | 满盘无五连局面不再搜索；Game 和棋检测 PASS |
| Restart | PASS | 胜负后重新开始并再次落子正常 |
| Cancel | PASS | 搜索中 cancel 约 121ms 安全返回 |
| Timeout | PASS | 5ms 限制实际 5.0026ms 安全返回 |
| Repeated games | PASS | AI Core 重复调用 x100；HvH/HvA 多轮冒烟测试 |

## 测试统计

### AI Core + Controller 测试

```text
Passed: 27
Failed: 0
```

- 合法落子 x100
- 立即获胜 / 立即防守 x4 方向
- 终局停止（黑胜 / 白胜 / 外部状态）
- 深度 1/2/3 安全返回
- 重复调用 x100
- Timeout：requested=5ms，actual=5.0026ms
- Cancel：elapsed=121.434ms
- Node Limit：200 节点内返回合法着法
- Iterative Deepening：完整深度 3
- Zobrist：同局面同哈希 / 落子变化 / 悔棋恢复
- TranspositionTable：存储 / 查询 / 清除
- 默认配置（TT+ID+MO）回归
- 满盘和棋终局
- GameController：HvH 胜局、HvA 交替、AI 先手
- Game 和棋检测

### UI 冒烟测试

```text
HVH_SMOKE_TEST = PASS
HVA_SMOKE_TEST（玩家先手） = PASS
HVA_SMOKE_TEST（AI 先手） = PASS
```

## 已知限制

- 当前环境 `g++ -E` 预处理不可用，AutoMoc 的 `moc_predefs.h` 生成会失败。
- 已用手动 `moc` + CMake 自定义命令替代，删除生成文件后仍可自动重建，已验证可复现构建。
- 该限制不影响程序运行与 AI 功能。

## 最终状态

```text
Build = PASS
Runtime = PASS
Human vs Human = PASS
Human vs AI = PASS
AI timeout = PASS
AI cancel = PASS
Win detection = PASS
Draw detection = PASS
Restart = PASS
Repeated games = PASS
```

```text
PROJECT = COMPLETE
D:\gomoku = 唯一开发目录
AI CORE = TESTED
HUMAN VS AI = PASS
```
