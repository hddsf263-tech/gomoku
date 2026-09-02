#ifndef EVALUATION_H
#define EVALUATION_H

#include <cstdint>
#include "../Board.h"

namespace Gomoku {

/// @brief 静态评估函数，纯计算，不依赖 UI
class Evaluation {
public:
    /// @brief 从 player 视角评估局面，正数有利
    static int64_t evaluate(const Board& board, ChessPiece player);

private:
    static int64_t scoreFor(const Board& board, ChessPiece player);
    static int runLength(const Board& board, int row, int col,
                         int dr, int dc, ChessPiece piece);
    static int openEnds(const Board& board, int row, int col,
                        int dr, int dc, int length, ChessPiece piece);
};

} // namespace Gomoku

#endif // EVALUATION_H
