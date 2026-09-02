#include "ai/TranspositionTable.h"

namespace Gomoku {

void TranspositionTable::clear() {
    entries.clear();
}

void TranspositionTable::store(uint64_t hash, int depth, int64_t score,
                               TTNodeType type, const Position& bestMove) {
    TTEntry entry;
    entry.hash = hash;
    entry.depth = depth;
    entry.score = score;
    entry.type = type;
    entry.bestMove = bestMove;
    entry.valid = true;
    entries[hash] = entry;
}

bool TranspositionTable::probe(uint64_t hash, int depth,
                               int64_t& score, TTNodeType& type,
                               Position& bestMove) const {
    auto it = entries.find(hash);
    if (it == entries.end() || !it->second.valid) {
        return false;
    }
    const TTEntry& entry = it->second;
    if (entry.depth < depth) {
        return false;
    }
    score = entry.score;
    type = entry.type;
    bestMove = entry.bestMove;
    return true;
}

} // namespace Gomoku
