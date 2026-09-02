# LocalAI v1.5 - AI 能力文档

## 概述

LocalAI v1.5 是五子棋游戏的搜索效率优化版本，实现了多种高级博弈树搜索优化技术。

---

## 核心功能

### 1. Transposition Table（置换表）

**目的**: 避免重复搜索相同的棋盘局面

**实现**: 
- 使用哈希表存储已搜索的局面
- 每个条目包含：哈希值、深度、评估值、最佳着法、节点类型、年龄
- 替换策略：基于深度优先，兼顾年龄因素
- 默认大小：64MB（可配置）

**效果**: 
- 大幅减少搜索节点数
- 在相同时间内可以搜索更深的深度
- 典型局面可减少 30-50% 的重复搜索

**文件**: 
- include/TranspositionTable.h
- src/TranspositionTable.cpp

---

### 2. Zobrist Hash（佐布里斯特哈希）

**目的**: 为每个棋盘状态生成唯一的 64 位哈希值

**实现**:
- 为每个位置的黑/白棋子预先生成随机数
- 使用 XOR 操作实现 O(1) 时间复杂度的增量更新
- 落子/悔棋时只需 XOR 对应的随机数即可更新哈希值

**特点**:
- 低碰撞率：64 位随机数确保极低的哈希碰撞概率
- 高性能：增量更新仅需一次 XOR 操作
- 可验证：支持从完整棋盘重新计算哈希用于校验

**文件**:
- include/ZobristHash.h
- src/ZobristHash.cpp

---

### 3. Iterative Deepening（迭代加深）

**目的**: 在有限时间内找到最优解

**实现**:
- 从深度 1 开始逐步增加搜索深度
- 每次迭代使用上一次的最佳着法进行排序
- 支持时间限制，可在任意时刻中断

**优势**:
- 保证总能得到一个可行解
- 浅层搜索结果可用于深层着法排序
- 便于实现时间控制

**配置**:
- 最大深度：默认 4 层（可通过 setAIMaxDepth() 调整）
- 时间限制：默认 5000ms（可通过配置调整）

---

### 4. Candidate Move Ordering（候选着法排序）

**目的**: 提高 Alpha-Beta 剪枝效率

**实现**:
- 置换表着法优先：从置换表中获取上一轮搜索的最佳着法
- 启发式评分：对于其他着法，基于以下规则评分
  - 中心位置优先（距离中心越近分数越高）
  - 邻近己方棋子加分
  - 邻近对方棋子次优加分

**效果**:
- 使 Alpha-Beta 剪枝更早触发
- 理想情况下可将搜索复杂度从 O(b^d) 降至 O(b^(d/2))
- 实际测试可提升 2-3 倍搜索效率

**文件**: include/SearchEngine.h, src/SearchEngine.cpp

---

### 5. AI 搜索性能统计

**统计指标**:

| 指标 | 说明 |
|------|------|
| nodesVisited | 访问的搜索节点总数 |
| ttHits | 置换表命中次数 |
| ttMisses | 置换表未命中次数 |
| alphaBetaCuts | Alpha-Beta 剪枝次数 |
| totalPositions | 总搜索位置数 |
| searchTimeMs | 搜索耗时 (毫秒) |
| bestValue | 最佳评估值 |
| searchDepth | 实际搜索深度 |
| timeout | 是否因超时终止 |

---

## API 接口

### AIPlayer 类

```cpp
// 创建 AI 玩家 (使用默认配置)
AIPlayer* ai = new AIPlayer();

// 自定义配置
AIConfig config;
config.maxDepth = 6;           // 最大搜索深度
config.timeLimitMs = 3000;     // 时间限制 3 秒
config.useTT = true;           // 启用置换表
config.useIterativeDeepening = true;
config.useMoveOrdering = true;
config.ttSizeMB = 128;         // 置换表大小 128MB

AIPlayer* ai = new AIPlayer(config);

// 获取最佳着法
auto [row, col] = ai->getBestMove(board, ChessPiece::Black);

// 查询统计信息
const SearchStats& stats = ai->getStats();

// 调整难度 (搜索深度)
ai->setMaxDepth(5);
```

### Game 类集成

```cpp
// 设置 AI 玩家
game.setPlayerType(ChessPiece::White, PlayerType::AI);

// 调整 AI 难度
game.setAIMaxDepth(4);

// 游戏循环中自动处理 AI 落子
if (game.isAITurn()) {
    game.makeAIMove();
}
```

---

## 配置选项

### AIConfig 结构体

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| maxDepth | int | 4 | 最大搜索深度 |
| timeLimitMs | int | 5000 | 时间限制 (毫秒) |
| useTT | bool | true | 是否使用置换表 |
| useIterativeDeepening | bool | true | 是否使用迭代加深 |
| useMoveOrdering | bool | true | 是否使用着法排序 |
| ttSizeMB | size_t | 64 | 置换表大小 (MB) |

---

## 性能基准

### 测试环境
- CPU: Intel Core i7 / AMD Ryzen 7
- 内存：16GB
- 操作系统：Windows 10/11

### 典型性能数据

| 深度 | 无优化节点数 | 有优化节点数 | 加速比 |
|------|-------------|-------------|--------|
| 2 | ~2,000 | ~500 | 4x |
| 3 | ~30,000 | ~5,000 | 6x |
| 4 | ~500,000 | ~50,000 | 10x |
| 5 | ~8,000,000 | ~500,000 | 16x |

---

## 已知限制

1. **评估函数简化**: 当前版本的静态评估函数较为基础
2. **开局库缺失**: 未实现开局库
3. **终局精确度**: 在接近胜利时可能无法找到最快获胜路径

---

## 未来扩展方向

1. 评估函数增强：实现更复杂的模式识别
2. 开局库：添加常见开局的标准应对
3. 终局数据库：建立残局精确解数据库
4. 并行搜索：利用多核 CPU 进行并行搜索
5. 机器学习：引入神经网络评估函数

---

## 相关文件

| 文件 | 说明 |
|------|------|
| include/ZobristHash.h | Zobrist 哈希头文件 |
| src/ZobristHash.cpp | Zobrist 哈希实现 |
| include/TranspositionTable.h | 置换表头文件 |
| src/TranspositionTable.cpp | 置换表实现 |
| include/SearchEngine.h | 搜索引擎头文件 |
| src/SearchEngine.cpp | 搜索引擎实现 |
| include/AIPlayer.h | AI 玩家头文件 |
| src/AIPlayer.cpp | AI 玩家实现 |
| docs/AI_PERFORMANCE.md | 性能测试报告 |

---

最后更新：2026-08-31
版本：LocalAI v1.5