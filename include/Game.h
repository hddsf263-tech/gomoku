#ifndef GAME_H
#define GAME_H

#include <functional>

#include "ChessPiece.h"
#include "Board.h"
#include "ai/GameStateSnapshot.h"

namespace Gomoku {

enum class MoveResult {
    Success,
    InvalidPosition,
    PositionOccupied,
    GameEnded
};

class Game {
public:
    Game();
    ~Game();

    void startNewGame();
    MoveResult makeMove(int row, int col);
    bool undoMove();

    GameState getState() const { return state; }
    ChessPiece getCurrentPlayer() const { return currentPlayer; }
    Board& getBoard() { return board; }
    const Board& getBoard() const { return board; }

    void setPlayerType(ChessPiece piece, PlayerType type);
    PlayerType getPlayerType(ChessPiece piece) const;
    bool isAITurn() const;
    bool isHumanVsAI() const;
    GameStateSnapshot getGameStateSnapshot() const;

    using StateCallback = std::function<void(GameState)>;
    void onStateChanged(StateCallback callback) { stateCallback = callback; }
    using MoveCallback = std::function<void(int, int)>;
    void onMoveMade(MoveCallback callback) { moveCallback = callback; }

private:
    void switchPlayer();
    void checkGameEnd(int row, int col);

    Board board;
    ChessPiece currentPlayer;
    GameState state;
    PlayerType blackPlayerType;
    PlayerType whitePlayerType;
    StateCallback stateCallback;
    MoveCallback moveCallback;
};

} // namespace Gomoku

#endif // GAME_H