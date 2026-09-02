#include "ai/ZobristHash.h"

#include <random>

namespace Gomoku {

void ZobristHash::init() {
    std::mt19937_64 rng(20260901);
    emptyHash = rng();
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            for (int i = 0; i < PIECE_TYPES; ++i) {
                table[row][col][i] = rng();
            }
        }
    }
    initialized = true;
}

uint64_t ZobristHash::computeHash(const Board& board) const {
    if (!initialized) {
        const_cast<ZobristHash*>(this)->init();
    }
    uint64_t hash = emptyHash;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            ChessPiece piece = board.getPiece(row, col);
            if (piece != ChessPiece::Empty) {
                hash ^= table[row][col][static_cast<int>(piece) - 1];
            }
        }
    }
    return hash;
}

uint64_t ZobristHash::pieceKey(int row, int col, ChessPiece piece) const {
    if (!initialized) {
        const_cast<ZobristHash*>(this)->init();
    }
    if (piece == ChessPiece::Empty) {
        return 0;
    }
    return table[row][col][static_cast<int>(piece) - 1];
}

} // namespace Gomoku
