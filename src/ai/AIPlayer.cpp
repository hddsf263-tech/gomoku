#include "ai/AIPlayer.h"

namespace Gomoku {

AIPlayer::AIPlayer(const AIConfig& config)
    : config(config) {
}

Move AIPlayer::getMove(const GameStateSnapshot& state) {
    search(state);
    return lastResult.move;
}

SearchResult AIPlayer::search(const GameStateSnapshot& state) {
    cancelFlag.store(false);
    lastResult = engine.findBestMove(state, config, &cancelFlag);
    return lastResult;
}

void AIPlayer::cancel() {
    cancelFlag.store(true);
}

void AIPlayer::setConfig(const AIConfig& newConfig) {
    config = newConfig;
}

} // namespace Gomoku
