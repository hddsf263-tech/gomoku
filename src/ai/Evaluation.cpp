#include "ai/Evaluation.h"

#include <cstdlib>

namespace Gomoku {

namespace {

constexpr int64_t SCORE_FIVE = 1000000;
constexpr int64_t SCORE_LIVE_FOUR = 100000;
constexpr int64_t SCORE_RUSH_FOUR = 10000;
constexpr int64_t SCORE_LIVE_THREE = 5000;
constexpr int64_t SCORE_SLEEP_THREE = 1000;
constexpr int64_t SCORE_LIVE_TWO = 500;
constexpr int64_t SCORE_SLEEP_TWO = 100;
constexpr int64_t SCORE_SINGLE = 20;

} // namespace

int64_t Evaluation::evaluate(const Board& board, ChessPiece player) {
    ChessPiece opponent =
        (player == ChessPiece::Black) ? ChessPiece::White : ChessPiece::Black;
    return scoreFor(board, player) - scoreFor(board, opponent);
}

int64_t Evaluation::scoreFor(const Board& board, ChessPiece player) {
    int64_t total = 0;
    static const int directions[4][2] = {
        {0, 1}, {1, 0}, {1, 1}, {1, -1}
    };

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            if (board.getPiece(row, col) != player) {
                continue;
            }

            int centerDist = std::abs(row - BOARD_SIZE / 2) +
                             std::abs(col - BOARD_SIZE / 2);
            total += (BOARD_SIZE - centerDist) * 2;

            for (const auto& dir : directions) {
                int dr = dir[0];
                int dc = dir[1];
                int prevRow = row - dr;
                int prevCol = col - dc;

                bool isRunStart =
                    prevRow < 0 || prevRow >= BOARD_SIZE ||
                    prevCol < 0 || prevCol >= BOARD_SIZE ||
                    board.getPiece(prevRow, prevCol) != player;
                if (!isRunStart) {
                    continue;
                }

                int length = runLength(board, row, col, dr, dc, player);
                int ends = openEnds(board, row, col, dr, dc, length, player);

                if (length >= 5) {
                    total += SCORE_FIVE;
                } else if (length == 4) {
                    total += (ends == 2) ? SCORE_LIVE_FOUR : SCORE_RUSH_FOUR;
                } else if (length == 3) {
                    total += (ends == 2) ? SCORE_LIVE_THREE : SCORE_SLEEP_THREE;
                } else if (length == 2) {
                    total += (ends == 2) ? SCORE_LIVE_TWO : SCORE_SLEEP_TWO;
                } else {
                    total += SCORE_SINGLE;
                }
            }
        }
    }

    return total;
}

int Evaluation::runLength(const Board& board, int row, int col,
                          int dr, int dc, ChessPiece piece) {
    int count = 1;
    int r = row + dr;
    int c = col + dc;
    while (r >= 0 && r < BOARD_SIZE &&
           c >= 0 && c < BOARD_SIZE &&
           board.getPiece(r, c) == piece) {
        ++count;
        r += dr;
        c += dc;
    }
    return count;
}

int Evaluation::openEnds(const Board& board, int row, int col,
                         int dr, int dc, int length, ChessPiece piece) {
    (void)piece;
    int ends = 0;

    int r = row - dr;
    int c = col - dc;
    if (r >= 0 && r < BOARD_SIZE &&
        c >= 0 && c < BOARD_SIZE &&
        board.isEmpty(r, c)) {
        ++ends;
    }

    int endRow = row + (length - 1) * dr;
    int endCol = col + (length - 1) * dc;
    r = endRow + dr;
    c = endCol + dc;
    if (r >= 0 && r < BOARD_SIZE &&
        c >= 0 && c < BOARD_SIZE &&
        board.isEmpty(r, c)) {
        ++ends;
    }

    return ends;
}

} // namespace Gomoku
