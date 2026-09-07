#include "GomokuCore.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>

namespace Gomoku {

namespace {

constexpr int kBoardSize = 15;
constexpr int kCellCount = kBoardSize * kBoardSize;

int otherPlayer(int player) {
    return player == 1 ? 2 : 1;
}

} // namespace

GameEngine::GameEngine() {
    reset();
}

void GameEngine::reset() {
    for (auto& row : board_) {
        row.fill(Piece::Empty);
    }
    history_.clear();
    current_ = Piece::Black;
    status_ = GameStatus::InProgress;
}

Piece GameEngine::pieceAt(int row, int col) const {
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize) {
        return Piece::Empty;
    }
    return board_[row][col];
}

bool GameEngine::canPlace(int row, int col) const {
    return status_ == GameStatus::InProgress &&
           row >= 0 && row < kBoardSize &&
           col >= 0 && col < kBoardSize &&
           board_[row][col] == Piece::Empty;
}

bool GameEngine::makeMove(int row, int col) {
    if (!canPlace(row, col)) {
        return false;
    }
    board_[row][col] = current_;
    history_.push_back({ row, col, current_ });
    updateStatusAfterMove(row, col);
    if (status_ == GameStatus::InProgress) {
        switchPlayer();
    }
    return true;
}

bool GameEngine::undo() {
    if (history_.empty()) {
        return false;
    }
    const GameMove last = history_.back();
    history_.pop_back();
    board_[last.row][last.col] = Piece::Empty;
    current_ = last.piece;
    status_ = GameStatus::InProgress;
    return true;
}

std::optional<GameMove> GameEngine::lastMove() const {
    if (history_.empty()) {
        return std::nullopt;
    }
    return history_.back();
}

bool GameEngine::isFiveAt(int row, int col, Piece piece) const {
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize ||
        piece == Piece::Empty) {
        return false;
    }

    static const int directions[4][2] = {
        { 0, 1 }, { 1, 0 }, { 1, 1 }, { 1, -1 }
    };

    for (const auto& dir : directions) {
        int count = 1;
        int r = row + dir[0];
        int c = col + dir[1];
        while (pieceAt(r, c) == piece) {
            count++;
            r += dir[0];
            c += dir[1];
        }
        r = row - dir[0];
        c = col - dir[1];
        while (pieceAt(r, c) == piece) {
            count++;
            r -= dir[0];
            c -= dir[1];
        }
        if (count >= 5) {
            return true;
        }
    }
    return false;
}

std::vector<GameMove> GameEngine::winningLine(int row, int col) const {
    const Piece piece = pieceAt(row, col);
    if (piece == Piece::Empty) {
        return {};
    }

    static const int directions[4][2] = {
        { 0, 1 }, { 1, 0 }, { 1, 1 }, { 1, -1 }
    };

    for (const auto& dir : directions) {
        std::vector<GameMove> line{ { row, col, piece } };
        for (int sign : { -1, 1 }) {
            int r = row + sign * dir[0];
            int c = col + sign * dir[1];
            while (pieceAt(r, c) == piece) {
                line.push_back({ r, c, piece });
                r += sign * dir[0];
                c += sign * dir[1];
            }
        }
        if (line.size() >= 5) {
            return line;
        }
    }
    return {};
}

void GameEngine::switchPlayer() {
    current_ = current_ == Piece::Black ? Piece::White : Piece::Black;
}

void GameEngine::updateStatusAfterMove(int row, int col) {
    const Piece piece = board_[row][col];
    if (isFiveAt(row, col, piece)) {
        status_ = piece == Piece::Black ? GameStatus::BlackWin : GameStatus::WhiteWin;
    } else if (history_.size() >= kCellCount) {
        status_ = GameStatus::Draw;
    }
}

namespace {

struct SearchBoard {
    std::array<int, kCellCount> cells{};

    int get(int row, int col) const {
        if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize) {
            return 0;
        }
        return cells[row * kBoardSize + col];
    }

    bool emptyAt(int row, int col) const {
        return row >= 0 && row < kBoardSize && col >= 0 && col < kBoardSize &&
               cells[row * kBoardSize + col] == 0;
    }

    int countStones() const {
        int count = 0;
        for (int cell : cells) {
            if (cell != 0) {
                count++;
            }
        }
        return count;
    }
};

constexpr int64_t kInf = 1000000000;
constexpr int64_t kScoreFive = 1000000;
constexpr int64_t kScoreLiveFour = 100000;
constexpr int64_t kScoreRushFour = 10000;
constexpr int64_t kScoreLiveThree = 5000;
constexpr int64_t kScoreSleepThree = 1000;
constexpr int64_t kScoreLiveTwo = 500;
constexpr int64_t kScoreSleepTwo = 100;
constexpr int64_t kScoreSingle = 20;

bool hasNearby(const SearchBoard& board, int row, int col, int radius) {
    for (int dr = -radius; dr <= radius; dr++) {
        for (int dc = -radius; dc <= radius; dc++) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            if (board.get(row + dr, col + dc) != 0) {
                return true;
            }
        }
    }
    return false;
}

void generateMoves(const SearchBoard& board, std::vector<GameMove>& moves) {
    moves.clear();
    const int stones = board.countStones();
    if (stones == 0) {
        moves.push_back({ 7, 7, Piece::Empty });
        return;
    }
    for (int row = 0; row < kBoardSize; row++) {
        for (int col = 0; col < kBoardSize; col++) {
            if (board.emptyAt(row, col) && hasNearby(board, row, col, 2)) {
                moves.push_back({ row, col, Piece::Empty });
            }
        }
    }
    if (moves.empty()) {
        for (int row = 0; row < kBoardSize; row++) {
            for (int col = 0; col < kBoardSize; col++) {
                if (board.emptyAt(row, col)) {
                    moves.push_back({ row, col, Piece::Empty });
                }
            }
        }
    }
}

int runLength(const SearchBoard& board, int row, int col,
              int dr, int dc, int piece) {
    int count = 1;
    int r = row + dr;
    int c = col + dc;
    while (board.get(r, c) == piece) {
        count++;
        r += dr;
        c += dc;
    }
    return count;
}

int openEnds(const SearchBoard& board, int row, int col,
             int dr, int dc, int length) {
    int ends = 0;
    if (board.emptyAt(row - dr, col - dc)) {
        ends++;
    }
    const int endRow = row + (length - 1) * dr;
    const int endCol = col + (length - 1) * dc;
    if (board.emptyAt(endRow + dr, endCol + dc)) {
        ends++;
    }
    return ends;
}

int64_t scoreFor(const SearchBoard& board, int player) {
    int64_t total = 0;
    static const int directions[4][2] = {
        { 0, 1 }, { 1, 0 }, { 1, 1 }, { 1, -1 }
    };
    for (int row = 0; row < kBoardSize; row++) {
        for (int col = 0; col < kBoardSize; col++) {
            if (board.get(row, col) != player) {
                continue;
            }
            const int centerDist = std::abs(row - 7) + std::abs(col - 7);
            total += (kBoardSize - centerDist) * 2;

            for (const auto& dir : directions) {
                const int dr = dir[0];
                const int dc = dir[1];
                if (board.get(row - dr, col - dc) == player) {
                    continue;
                }
                const int length = runLength(board, row, col, dr, dc, player);
                const int ends = openEnds(board, row, col, dr, dc, length);
                if (length >= 5) {
                    total += kScoreFive;
                } else if (length == 4) {
                    total += ends == 2 ? kScoreLiveFour : kScoreRushFour;
                } else if (length == 3) {
                    total += ends == 2 ? kScoreLiveThree : kScoreSleepThree;
                } else if (length == 2) {
                    total += ends == 2 ? kScoreLiveTwo : kScoreSleepTwo;
                } else {
                    total += kScoreSingle;
                }
            }
        }
    }
    return total;
}

int64_t evaluate(const SearchBoard& board, int player) {
    return scoreFor(board, player) - scoreFor(board, otherPlayer(player));
}

bool fiveAt(const SearchBoard& board, int row, int col, int piece) {
    static const int directions[4][2] = {
        { 0, 1 }, { 1, 0 }, { 1, 1 }, { 1, -1 }
    };
    for (const auto& dir : directions) {
        int count = 1;
        int r = row + dir[0];
        int c = col + dir[1];
        while (board.get(r, c) == piece) {
            count++;
            r += dir[0];
            c += dir[1];
        }
        r = row - dir[0];
        c = col - dir[1];
        while (board.get(r, c) == piece) {
            count++;
            r -= dir[0];
            c -= dir[1];
        }
        if (count >= 5) {
            return true;
        }
    }
    return false;
}

void sortMoves(std::vector<GameMove>& moves, const SearchBoard& board) {
    for (GameMove& move : moves) {
        int score = (kBoardSize - (std::abs(move.row - 7) + std::abs(move.col - 7))) * 10;
        for (int dr = -2; dr <= 2; dr++) {
            for (int dc = -2; dc <= 2; dc++) {
                if (dr == 0 && dc == 0) {
                    continue;
                }
                if (board.get(move.row + dr, move.col + dc) != 0) {
                    score += 30;
                }
            }
        }
        move.sortScore = score;
    }
    std::stable_sort(moves.begin(), moves.end(),
                     [](const GameMove& a, const GameMove& b) {
        return a.sortScore > b.sortScore;
    });
}

SearchBoard fromGame(const GameEngine& game) {
    SearchBoard board;
    for (int row = 0; row < kBoardSize; row++) {
        for (int col = 0; col < kBoardSize; col++) {
            board.cells[row * kBoardSize + col] =
                static_cast<int>(game.pieceAt(row, col));
        }
    }
    return board;
}

} // namespace

AiResult AiSearch::findBestMove(const GameEngine& game,
                                Piece player,
                                const AiConfig& config,
                                const std::function<bool()>& cancel) {
    AiResult result;
    SearchBoard rootBoard = fromGame(game);
    const int stones = rootBoard.countStones();
    const auto startTime = std::chrono::steady_clock::now();
    int64_t nodesVisited = 0;
    bool aborted = false;

    const auto elapsedMs = [&]() {
        return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - startTime).count();
    };
    const auto shouldStop = [&]() {
        if (aborted) {
            return true;
        }
        if (cancel && cancel()) {
            aborted = true;
            return true;
        }
        if (config.maxNodes > 0 && nodesVisited >= config.maxNodes) {
            aborted = true;
            return true;
        }
        if (config.timeLimitMs > 0 && elapsedMs() >= config.timeLimitMs) {
            aborted = true;
            return true;
        }
        return false;
    };

    std::function<int64_t(SearchBoard, int, int, int, int, int, int, int)> negamax;
    negamax = [&](SearchBoard board, int lastRow, int lastCol, int lastPiece,
                  int turn, int depth, int64_t alpha, int64_t beta) -> int64_t {
        nodesVisited++;
        if (lastRow >= 0 && fiveAt(board, lastRow, lastCol, lastPiece)) {
            return lastPiece == turn ? kInf : -kInf;
        }
        const int placed = board.countStones();
        if (placed >= kCellCount) {
            return 0;
        }
        if (depth <= 0) {
            return evaluate(board, turn);
        }

        std::vector<GameMove> moves;
        generateMoves(board, moves);
        if (moves.empty()) {
            return 0;
        }
        sortMoves(moves, board);

        int64_t best = -kInf;
        for (const GameMove& move : moves) {
            if (shouldStop()) {
                return 0;
            }
            board.cells[move.row * kBoardSize + move.col] = turn;
            const int64_t value = -negamax(
                board, move.row, move.col, turn,
                otherPlayer(turn), depth - 1, -beta, -alpha);
            board.cells[move.row * kBoardSize + move.col] = 0;
            best = std::max(best, value);
            alpha = std::max(alpha, best);
            if (alpha >= beta) {
                break;
            }
        }
        return best;
    };

    std::vector<GameMove> rootMoves;
    generateMoves(rootBoard, rootMoves);
    if (rootMoves.empty()) {
        result.timeMs = elapsedMs();
        return result;
    }
    sortMoves(rootMoves, rootBoard);

    GameMove bestMove = rootMoves.front();
    int64_t bestValue = -kInf;
    int completedDepth = 0;

    for (int depth = 1; depth <= config.maxDepth && !aborted; depth++) {
        if (shouldStop()) {
            aborted = true;
            break;
        }
        GameMove depthMove = bestMove;
        int64_t depthBest = bestValue;
        for (const GameMove& move : rootMoves) {
            if (shouldStop()) {
                aborted = true;
                break;
            }
            SearchBoard child = rootBoard;
            child.cells[move.row * kBoardSize + move.col] = static_cast<int>(player);
            const int64_t value = -negamax(
                child, move.row, move.col, static_cast<int>(player),
                otherPlayer(static_cast<int>(player)),
                depth - 1, -kInf, kInf);
            if (value > depthBest) {
                depthBest = value;
                depthMove = move;
            }
        }
        if (aborted) {
            break;
        }
        bestMove = depthMove;
        bestValue = depthBest;
        completedDepth = depth;
    }

    result.move = bestMove;
    result.valid = game.canPlace(bestMove.row, bestMove.col);
    result.completedDepth = completedDepth;
    result.nodesVisited = nodesVisited;
    result.timeMs = elapsedMs();
    result.timedOut = config.timeLimitMs > 0 && result.timeMs >= config.timeLimitMs;
    return result;
}

} // namespace Gomoku
