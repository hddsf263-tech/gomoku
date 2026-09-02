#ifndef SEARCH_RESULT_H
#define SEARCH_RESULT_H

#include <cstdint>
#include "../Board.h"

namespace Gomoku {

/// @brief 一次搜索的完整结果与统计
struct SearchResult {
    Position move{-1, -1};
    bool valid = false;
    bool timedOut = false;
    bool cancelled = false;
    bool nodeLimitReached = false;
    int completedDepth = 0;
    uint64_t nodesVisited = 0;
    int64_t bestValue = 0;
    double timeMs = 0.0;
};

} // namespace Gomoku

#endif // SEARCH_RESULT_H
