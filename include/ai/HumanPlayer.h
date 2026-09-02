#ifndef HUMAN_PLAYER_H
#define HUMAN_PLAYER_H

#include "ai/IPlayer.h"

namespace Gomoku {

/// @brief 人类玩家适配器（对称接口；实际 UI 流程由点击事件驱动）
class HumanPlayer : public IPlayer {
public:
    void setMove(const Move& move);

    Move getMove(const GameStateSnapshot& state) override;

private:
    Move pendingMove{-1, -1};
    bool hasPendingMove = false;
};

} // namespace Gomoku

#endif // HUMAN_PLAYER_H
