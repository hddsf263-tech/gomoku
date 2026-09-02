#include "ai/MoveGenerator.h"

namespace Gomoku {

std::vector<Position> MoveGenerator::generateMoves(const Board& board) {
    std::vector<Position> moves;

    if (board.getMoveHistory().empty()) {
        moves.push_back({BOARD_SIZE / 2, BOARD_SIZE / 2});
        return moves;
    }

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            if (board.isEmpty(row, col) &&
                hasPieceNearby(board, row, col, 2)) {
                moves.push_back({row, col});
            }
        }
    }

    // 防御性回退：候选为空时返回全部空位
    if (moves.empty()) {
        for (int row = 0; row < BOARD_SIZE; ++row) {
            for (int col = 0; col < BOARD_SIZE; ++col) {
                if (board.isEmpty(row, col)) {
                    moves.push_back({row, col});
                }
            }
        }
    }

    return moves;
}

bool MoveGenerator::hasPieceNearby(const Board& board,
                                   int row, int col, int radius) {
    for (int dr = -radius; dr <= radius; ++dr) {
        for (int dc = -radius; dc <= radius; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            int nr = row + dr;
            int nc = col + dc;
            if (nr >= 0 && nr < BOARD_SIZE &&
                nc >= 0 && nc < BOARD_SIZE &&
                !board.isEmpty(nr, nc)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace Gomoku
