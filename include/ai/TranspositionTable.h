#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <cstdint>
#include <unordered_map>

#include "../Board.h"

namespace Gomoku {

/// @brief TT 节点类型
enum class TTNodeType {
    Exact,
    LowerBound,
    UpperBound
};

struct TTEntry {
    uint64_t hash = 0;
    int depth = -1;
    int64_t score = 0;
    TTNodeType type = TTNodeType::Exact;
    Position bestMove{-1, -1};
    bool valid = false;
};

/// @brief 置换表，以 ZobristHash 为 key
class TranspositionTable {
public:
    void clear();

    void store(uint64_t hash, int depth, int64_t score,
               TTNodeType type, const Position& bestMove);

    bool probe(uint64_t hash, int depth,
               int64_t& score, TTNodeType& type,
               Position& bestMove) const;

    size_t size() const { return entries.size(); }

private:
    std::unordered_map<uint64_t, TTEntry> entries;
};

} // namespace Gomoku

#endif // TRANSPOSITION_TABLE_H
