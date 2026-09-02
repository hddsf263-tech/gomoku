#ifndef ZOBRIST_HASH_H
#define ZOBRIST_HASH_H

#include <cstdint>

#include "../Board.h"
#include "../ChessPiece.h"

namespace Gomoku {

/// @brief 64 位 Zobrist 棋盘哈希
class ZobristHash {
public:
    ZobristHash() = default;

    /// @brief 初始化随机数表（固定种子，保证可复现）
    void init();

    /// @brief 从完整棋盘计算哈希
    uint64_t computeHash(const Board& board) const;

    /// @brief 返回某个位置的棋子键值，用于增量更新
    uint64_t pieceKey(int row, int col, ChessPiece piece) const;

    uint64_t getEmptyHash() const { return emptyHash; }

private:
    static constexpr int PIECE_TYPES = 2; // Black / White

    uint64_t table[BOARD_SIZE][BOARD_SIZE][PIECE_TYPES]{};
    uint64_t emptyHash = 0;
    bool initialized = false;
};

} // namespace Gomoku

#endif // ZOBRIST_HASH_H
