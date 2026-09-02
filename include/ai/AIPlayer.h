#ifndef AI_PLAYER_H
#define AI_PLAYER_H

#include <atomic>

#include "ai/AIConfig.h"
#include "ai/IPlayer.h"
#include "ai/SearchEngine.h"
#include "ai/SearchResult.h"

namespace Gomoku {

/// @brief AI 玩家：接收快照 -> SearchEngine -> Move
class AIPlayer : public IPlayer {
public:
    explicit AIPlayer(const AIConfig& config = AIConfig());

    Move getMove(const GameStateSnapshot& state) override;

    /// @brief 执行一次搜索并返回完整结果
    SearchResult search(const GameStateSnapshot& state);

    void cancel() override;

    void setConfig(const AIConfig& config);
    const AIConfig& getConfig() const { return config; }

private:
    AIConfig config;
    SearchEngine engine;
    std::atomic<bool> cancelFlag{false};
    SearchResult lastResult;
};

} // namespace Gomoku

#endif // AI_PLAYER_H
