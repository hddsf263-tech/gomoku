#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <vector>
#include "../Board.h"

namespace Gomoku {

/// @brief 候选着法生成器，只生成合法空位
class MoveGenerator {
public:
    /// @brief 生成当前局面的候选着法
    /// @details 有棋子时只生成棋子附近半径 2 的空位，保证不遗漏直接胜/防点
    static std::vector<Position> generateMoves(const Board& board);

private:
    static bool hasPieceNearby(const Board& board, int row, int col, int radius);
};

} // namespace Gomoku

#endif // MOVE_GENERATOR_H
