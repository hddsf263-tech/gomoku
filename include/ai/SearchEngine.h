#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

#include "ai/AIConfig.h"
#include "ai/GameStateSnapshot.h"
#include "ai/SearchResult.h"
#include "ai/TranspositionTable.h"
#include "ai/ZobristHash.h"

namespace Gomoku {

/// @brief Minimax + Alpha-Beta 搜索引擎（Phase 2C：TT / Zobrist / Move Ordering）
class SearchEngine {
public:
    static constexpr int64_t INF = 1000000000;

    SearchResult findBestMove(const GameStateSnapshot& state,
                              const AIConfig& config,
                              std::atomic<bool>* cancel = nullptr);

private:
    int64_t searchAtDepth(const GameStateSnapshot& state,
                          uint64_t rootHash,
                          int depth,
                          const AIConfig& config,
                          std::atomic<bool>* cancel,
                          Position& bestMove);

    int64_t negamax(Board board, uint64_t hash,
                    ChessPiece player, int depth,
                    int64_t alpha, int64_t beta,
                    const AIConfig& config,
                    std::atomic<bool>* cancel);

    void sortMoves(std::vector<Position>& moves,
                   uint64_t hash, const Board& board);

    bool shouldAbort(const AIConfig& config,
                     std::atomic<bool>* cancel) const;

    std::chrono::steady_clock::time_point startTime;
    uint64_t nodesVisited = 0;
    bool aborted = false;
    ZobristHash zobrist;
    TranspositionTable tt;
};

} // namespace Gomoku

#endif // SEARCH_ENGINE_H