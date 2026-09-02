#ifndef GAME_STATE_SNAPSHOT_H
#define GAME_STATE_SNAPSHOT_H

#include "../ChessPiece.h"
#include "../Board.h"

namespace Gomoku {

/// @brief AI 只读游戏状态快照，与 Game / UI 完全解耦
struct GameStateSnapshot {
    Board board;
    ChessPiece currentPlayer = ChessPiece::Black;
    GameState status = GameState::InProgress;
    int moveCount = 0;

    /// @brief 是否已经结束（胜利、和棋或外部状态结束）
    bool isTerminal() const {
        if (status != GameState::InProgress) {
            return true;
        }
        if (moveCount >= BOARD_SIZE * BOARD_SIZE) {
            return true;
        }
        auto last = board.getLastMove();
        if (last.has_value()) {
            ChessPiece piece = board.getPiece(last->row, last->col);
            if (piece != ChessPiece::Empty &&
                board.checkFiveInRow(last->row, last->col, piece)) {
                return true;
            }
        }
        return false;
    }

    /// @brief 返回获胜棋子；未结束或无胜者返回 Empty
    ChessPiece terminalWinner() const {
        auto last = board.getLastMove();
        if (last.has_value()) {
            ChessPiece piece = board.getPiece(last->row, last->col);
            if (piece != ChessPiece::Empty &&
                board.checkFiveInRow(last->row, last->col, piece)) {
                return piece;
            }
        }
        return ChessPiece::Empty;
    }
};

} // namespace Gomoku

#endif // GAME_STATE_SNAPSHOT_H
