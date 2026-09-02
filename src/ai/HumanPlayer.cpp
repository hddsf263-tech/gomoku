#include "ai/HumanPlayer.h"

namespace Gomoku {

void HumanPlayer::setMove(const Move& move) {
    pendingMove = move;
    hasPendingMove = true;
}

Move HumanPlayer::getMove(const GameStateSnapshot& state) {
    (void)state;
    if (!hasPendingMove) {
        return {-1, -1};
    }
    Move move = pendingMove;
    hasPendingMove = false;
    return move;
}

} // namespace Gomoku
