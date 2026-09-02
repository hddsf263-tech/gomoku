#ifndef AI_CONFIG_H
#define AI_CONFIG_H

#include <cstddef>
#include <cstdint>

namespace Gomoku {

/// @brief AI 搜索配置，所有搜索参数集中管理
struct AIConfig {
    int maxDepth = 4;
    int timeLimitMs = 2000;
    uint64_t maxNodes = 1000000;
    bool useTT = true;
    bool useIterativeDeepening = true;
    bool useMoveOrdering = true;
    size_t ttSizeMB = 64;
};

} // namespace Gomoku

#endif // AI_CONFIG_H