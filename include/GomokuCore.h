#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace Gomoku {

enum class Piece : int {
    Empty = 0,
    Black = 1,
    White = 2
};

enum class GameStatus {
    InProgress,
    BlackWin,
    WhiteWin,
    Draw
};

enum class PlayerKind {
    Human,
    AI
};

struct GameMove {
    int row = -1;
    int col = -1;
    Piece piece = Piece::Empty;
    int sortScore = 0;
};

class GameEngine {
public:
    GameEngine();

    void reset();
    bool makeMove(int row, int col);
    bool undo();

    Piece pieceAt(int row, int col) const;
    Piece currentPlayer() const { return current_; }
    GameStatus status() const { return status_; }
    int moveCount() const { return static_cast<int>(history_.size()); }
    const std::vector<GameMove>& history() const { return history_; }
    std::optional<GameMove> lastMove() const;

    bool isFiveAt(int row, int col, Piece piece) const;
    std::vector<GameMove> winningLine(int row, int col) const;
    bool canPlace(int row, int col) const;

private:
    void switchPlayer();
    void updateStatusAfterMove(int row, int col);

    std::array<std::array<Piece, 15>, 15> board_{};
    std::vector<GameMove> history_;
    Piece current_ = Piece::Black;
    GameStatus status_ = GameStatus::InProgress;
};

struct AiConfig {
    int maxDepth = 4;
    int timeLimitMs = 2000;
    int maxNodes = 1000000;
};

struct AiResult {
    GameMove move;
    bool valid = false;
    bool timedOut = false;
    int completedDepth = 0;
    int64_t nodesVisited = 0;
    double timeMs = 0.0;
};

class AiSearch {
public:
    static AiResult findBestMove(const GameEngine& game,
                                 Piece player,
                                 const AiConfig& config,
                                 const std::function<bool()>& cancel = nullptr);
};

} // namespace Gomoku
