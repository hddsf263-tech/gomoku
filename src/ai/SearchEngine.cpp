#include "ai/SearchEngine.h"

#include <algorithm>
#include <cstdlib>

#include "ai/MoveGenerator.h"
#include "ai/Evaluation.h"

namespace Gomoku {

namespace {

ChessPiece opponentOf(ChessPiece player) {
    return (player == ChessPiece::Black) ? ChessPiece::White : ChessPiece::Black;
}

bool terminalScore(const Board& board, ChessPiece player, int64_t& score) {
    auto last = board.getLastMove();
    if (last.has_value()) {
        ChessPiece piece = board.getPiece(last->row, last->col);
        if (piece != ChessPiece::Empty &&
            board.checkFiveInRow(last->row, last->col, piece)) {
            score = (piece == player) ? SearchEngine::INF : -SearchEngine::INF;
            return true;
        }
    }
    if (board.getMoveHistory().size() >=
        static_cast<size_t>(BOARD_SIZE * BOARD_SIZE)) {
        score = 0;
        return true;
    }
    return false;
}

} // namespace

SearchResult SearchEngine::findBestMove(const GameStateSnapshot& state,
                                        const AIConfig& config,
                                        std::atomic<bool>* cancel) {
    SearchResult result;
    startTime = std::chrono::steady_clock::now();
    nodesVisited = 0;
    aborted = false;
    tt.clear();

    if (state.isTerminal()) {
        result.timeMs = 0.0;
        return result;
    }

    auto moves = MoveGenerator::generateMoves(state.board);
    if (moves.empty()) {
        result.timeMs = 0.0;
        return result;
    }

    uint64_t rootHash = zobrist.computeHash(state.board);
    Position bestMove = moves.front();
    int64_t bestValue = -INF;
    int completedDepth = 0;

    if (config.useIterativeDeepening) {
        for (int depth = 1; depth <= config.maxDepth && !aborted; ++depth) {
            if (shouldAbort(config, cancel)) {
                aborted = true;
                break;
            }
            Position depthMove = moves.front();
            int64_t depthValue =
                searchAtDepth(state, rootHash, depth, config, cancel, depthMove);
            if (aborted) {
                break;
            }
            bestMove = depthMove;
            bestValue = depthValue;
            completedDepth = depth;
        }
    } else {
        bestValue = searchAtDepth(state, rootHash, config.maxDepth,
                                  config, cancel, bestMove);
        if (!aborted) {
            completedDepth = config.maxDepth;
        }
    }

    result.move = bestMove;
    result.valid =
        state.board.isValidPosition(bestMove.row, bestMove.col) &&
        state.board.isEmpty(bestMove.row, bestMove.col);
    result.completedDepth = completedDepth;
    result.nodesVisited = nodesVisited;
    result.bestValue = bestValue;
    result.cancelled = (cancel != nullptr && cancel->load());

    double elapsedMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - startTime).count();
    result.timedOut =
        (config.timeLimitMs > 0 && elapsedMs >= config.timeLimitMs);
    result.nodeLimitReached =
        (config.maxNodes > 0 && nodesVisited >= config.maxNodes);
    result.timeMs = elapsedMs;
    return result;
}

int64_t SearchEngine::searchAtDepth(const GameStateSnapshot& state,
                                    uint64_t rootHash,
                                    int depth,
                                    const AIConfig& config,
                                    std::atomic<bool>* cancel,
                                    Position& bestMove) {
    auto moves = MoveGenerator::generateMoves(state.board);
    if (moves.empty()) {
        bestMove = {-1, -1};
        return 0;
    }

    if (config.useMoveOrdering) {
        sortMoves(moves, rootHash, state.board);
    }

    int64_t best = -INF;
    Position bestPos = moves.front();

    for (const auto& move : moves) {
        if (shouldAbort(config, cancel)) {
            aborted = true;
            break;
        }

        Board child = state.board;
        child.placePiece(move.row, move.col, state.currentPlayer);
        uint64_t childHash =
            rootHash ^ zobrist.pieceKey(move.row, move.col, state.currentPlayer);
        int64_t value = -negamax(
            child, childHash, opponentOf(state.currentPlayer),
            depth - 1, -INF, INF, config, cancel);

        if (value > best) {
            best = value;
            bestPos = move;
        }
    }

    bestMove = bestPos;
    return best;
}

int64_t SearchEngine::negamax(Board board, uint64_t hash,
                              ChessPiece player, int depth,
                              int64_t alpha, int64_t beta,
                              const AIConfig& config,
                              std::atomic<bool>* cancel) {
    ++nodesVisited;

    if (shouldAbort(config, cancel)) {
        aborted = true;
        return 0;
    }

    int64_t score = 0;
    if (terminalScore(board, player, score)) {
        return score;
    }

    if (config.useTT) {
        int64_t ttScore = 0;
        TTNodeType ttType = TTNodeType::Exact;
        Position ttMove{-1, -1};
        if (tt.probe(hash, depth, ttScore, ttType, ttMove)) {
            if (ttType == TTNodeType::Exact) {
                return ttScore;
            }
            if (ttType == TTNodeType::LowerBound && ttScore >= beta) {
                return ttScore;
            }
            if (ttType == TTNodeType::UpperBound && ttScore <= alpha) {
                return ttScore;
            }
        }
    }

    if (depth <= 0) {
        return Evaluation::evaluate(board, player);
    }

    auto moves = MoveGenerator::generateMoves(board);
    if (moves.empty()) {
        return 0;
    }

    if (config.useMoveOrdering) {
        sortMoves(moves, hash, board);
    }

    int64_t best = -INF;
    Position bestPos = moves.front();

    for (const auto& move : moves) {
        if (shouldAbort(config, cancel)) {
            aborted = true;
            break;
        }

        Board child = board;
        child.placePiece(move.row, move.col, player);
        uint64_t childHash = hash ^ zobrist.pieceKey(move.row, move.col, player);
        int64_t value = -negamax(
            child, childHash, opponentOf(player), depth - 1,
            -beta, -alpha, config, cancel);

        if (value > best) {
            best = value;
            bestPos = move;
        }
        if (best > alpha) {
            alpha = best;
        }
        if (alpha >= beta) {
            break;
        }
    }

    if (config.useTT && !aborted) {
        TTNodeType nodeType = TTNodeType::Exact;
        if (best <= alpha) {
            nodeType = TTNodeType::UpperBound;
        } else if (best >= beta) {
            nodeType = TTNodeType::LowerBound;
        }
        tt.store(hash, depth, best, nodeType, bestPos);
    }

    return best;
}

void SearchEngine::sortMoves(std::vector<Position>& moves,
                             uint64_t hash, const Board& board) {
    int64_t ttScore = 0;
    TTNodeType ttType = TTNodeType::Exact;
    Position ttMove{-1, -1};
    bool hasTtMove = tt.probe(hash, 0, ttScore, ttType, ttMove);

    for (auto& move : moves) {
        int score = 0;
        if (hasTtMove && move.row == ttMove.row && move.col == ttMove.col) {
            score = 100000000;
        } else {
            int centerDist =
                std::abs(move.row - BOARD_SIZE / 2) +
                std::abs(move.col - BOARD_SIZE / 2);
            score += (BOARD_SIZE - centerDist) * 10;
            for (int dr = -2; dr <= 2; ++dr) {
                for (int dc = -2; dc <= 2; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int nr = move.row + dr;
                    int nc = move.col + dc;
                    if (nr >= 0 && nr < BOARD_SIZE &&
                        nc >= 0 && nc < BOARD_SIZE &&
                        !board.isEmpty(nr, nc)) {
                        score += 30;
                    }
                }
            }
        }
        move.score = score;
    }

    std::sort(moves.begin(), moves.end(), [](const Position& a, const Position& b) {
        return a.score > b.score;
    });
}

bool SearchEngine::shouldAbort(const AIConfig& config,
                               std::atomic<bool>* cancel) const {
    if (cancel != nullptr && cancel->load()) {
        return true;
    }
    if (config.maxNodes > 0 && nodesVisited >= config.maxNodes) {
        return true;
    }
    if (config.timeLimitMs > 0) {
        auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= config.timeLimitMs) {
            return true;
        }
    }
    return false;
}

} // namespace Gomoku