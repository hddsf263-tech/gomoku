#include "Game.h"

namespace Gomoku {

Game::Game()
    : currentPlayer(ChessPiece::Black)
    , state(GameState::NotStarted)
    , blackPlayerType(PlayerType::Human)
    , whitePlayerType(PlayerType::Human)
    , stateCallback(nullptr)
    , moveCallback(nullptr)
{
}

Game::~Game() {
}

void Game::startNewGame() {
    board.init();
    currentPlayer = ChessPiece::Black;
    state = GameState::InProgress;

    if (stateCallback) {
        stateCallback(state);
    }
}

MoveResult Game::makeMove(int row, int col) {
    if (state != GameState::InProgress) {
        return MoveResult::GameEnded;
    }

    if (!board.isValidPosition(row, col)) {
        return MoveResult::InvalidPosition;
    }

    if (!board.isEmpty(row, col)) {
        return MoveResult::PositionOccupied;
    }

    board.placePiece(row, col, currentPlayer);

    if (moveCallback) {
        moveCallback(row, col);
    }

    checkGameEnd(row, col);

    if (state == GameState::InProgress) {
        switchPlayer();
    }

    return MoveResult::Success;
}

bool Game::undoMove() {
    if (state == GameState::NotStarted) {
        return false;
    }

    bool success = board.undoLastMove();
    if (success) {
        if (board.getMoveHistory().empty()) {
            state = GameState::NotStarted;
            currentPlayer = ChessPiece::Black;
        } else {
            switchPlayer();
            state = GameState::InProgress;
        }

        if (stateCallback) {
            stateCallback(state);
        }
    }

    return success;
}

void Game::setPlayerType(ChessPiece piece, PlayerType type) {
    if (piece == ChessPiece::Black) {
        blackPlayerType = type;
    } else {
        whitePlayerType = type;
    }
}

PlayerType Game::getPlayerType(ChessPiece piece) const {
    return (piece == ChessPiece::Black) ? blackPlayerType : whitePlayerType;
}

bool Game::isAITurn() const {
    if (state != GameState::InProgress) {
        return false;
    }
    return getPlayerType(currentPlayer) == PlayerType::AI;
}
bool Game::isHumanVsAI() const {
    return (blackPlayerType == PlayerType::Human && whitePlayerType == PlayerType::AI) ||
           (blackPlayerType == PlayerType::AI && whitePlayerType == PlayerType::Human);
}

GameStateSnapshot Game::getGameStateSnapshot() const {
    GameStateSnapshot snapshot;
    snapshot.board = board;
    snapshot.currentPlayer = currentPlayer;
    snapshot.status = state;
    snapshot.moveCount = static_cast<int>(board.getMoveHistory().size());
    return snapshot;
}

void Game::switchPlayer() {
    currentPlayer = (currentPlayer == ChessPiece::Black) ? ChessPiece::White : ChessPiece::Black;
}

void Game::checkGameEnd(int row, int col) {
    ChessPiece piece = board.getPiece(row, col);

    if (board.checkFiveInRow(row, col, piece)) {
        state = (piece == ChessPiece::Black) ? GameState::BlackWin : GameState::WhiteWin;

        if (stateCallback) {
            stateCallback(state);
        }
        return;
    }

    if (board.getMoveHistory().size() >=
        static_cast<size_t>(BOARD_SIZE * BOARD_SIZE)) {
        state = GameState::Draw;

        if (stateCallback) {
            stateCallback(state);
        }
    }
}

} // namespace Gomoku