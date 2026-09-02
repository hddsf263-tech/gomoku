#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "Game.h"
#include "ai/AIPlayer.h"
#include "ai/SearchResult.h"

namespace Gomoku {

/// @brief 游戏控制器：组织 Human/AI 回合，不依赖 UI
class GameController {
public:
    void startGame(PlayerType blackPlayer, PlayerType whitePlayer);

    MoveResult applyHumanMove(int row, int col);

    bool isAITurn() const { return game.isAITurn(); }

    /// @brief 同步执行一次 AI 回合
    SearchResult runAIMove();

    void cancelAI() { aiPlayer.cancel(); }

    GameState getState() const { return game.getState(); }
    const Game& getGame() const { return game; }
    Game& getGame() { return game; }

private:
    Game game;
    AIPlayer aiPlayer;
};

} // namespace Gomoku

#endif // GAME_CONTROLLER_H
