#include "GameController.h"

namespace Gomoku {

void GameController::startGame(PlayerType blackPlayer,
                               PlayerType whitePlayer) {
    game.startNewGame();
    game.setPlayerType(ChessPiece::Black, blackPlayer);
    game.setPlayerType(ChessPiece::White, whitePlayer);
}

MoveResult GameController::applyHumanMove(int row, int col) {
    return game.makeMove(row, col);
}

SearchResult GameController::runAIMove() {
    SearchResult result;
    if (!game.isAITurn()) {
        return result;
    }

    GameStateSnapshot snapshot = game.getGameStateSnapshot();
    result = aiPlayer.search(snapshot);

    if (result.valid) {
        MoveResult moveResult =
            game.makeMove(result.move.row, result.move.col);
        if (moveResult != MoveResult::Success) {
            result.valid = false;
        }
    }
    return result;
}

} // namespace Gomoku
