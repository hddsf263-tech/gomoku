#ifndef I_PLAYER_H
#define I_PLAYER_H

#include "../Board.h"
#include "ai/GameStateSnapshot.h"

namespace Gomoku {

/// @brief AI 使用的棋步类型，与 Board 坐标一致
using Move = Position;

/// @brief 玩家抽象接口
class IPlayer {
public:
    virtual ~IPlayer() = default;

    /// @brief 根据快照返回落子
    virtual Move getMove(const GameStateSnapshot& state) = 0;

    /// @brief 取消正在进行的思考
    virtual void cancel() {}
};

} // namespace Gomoku

#endif // I_PLAYER_H
