#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <thread>
#include <tuple>
#include <vector>

#include "Board.h"
#include "ChessPiece.h"
#include "ai/AIConfig.h"
#include "ai/Evaluation.h"
#include "ai/GameStateSnapshot.h"
#include "ai/MoveGenerator.h"
#include "ai/SearchEngine.h"
#include "ai/SearchResult.h"
#include "ai/TranspositionTable.h"
#include "ai/ZobristHash.h"
#include "ai/AIPlayer.h"
#include "ai/HumanPlayer.h"
#include "GameController.h"

using namespace Gomoku;

namespace {

int g_passed = 0;
int g_failed = 0;

void check(bool condition, const std::string& name) {
    if (condition) {
        ++g_passed;
        std::cout << "PASS  " << name << "\n";
    } else {
        ++g_failed;
        std::cout << "FAIL  " << name << "\n";
    }
}

Board makeBoard(const std::vector<std::tuple<int, int, ChessPiece>>& moves) {
    Board board;
    board.init();
    for (const auto& m : moves) {
        board.placePiece(std::get<0>(m), std::get<1>(m), std::get<2>(m));
    }
    return board;
}

GameStateSnapshot makeSnapshot(const Board& board, ChessPiece player) {
    GameStateSnapshot snap;
    snap.board = board;
    snap.currentPlayer = player;
    snap.status = GameState::InProgress;
    snap.moveCount = static_cast<int>(board.getMoveHistory().size());
    return snap;
}

Board randomBoard(std::mt19937& rng, int maxMoves) {
    Board board;
    board.init();
    int moves = static_cast<int>(rng() % static_cast<unsigned>(maxMoves + 1));
    ChessPiece color = ChessPiece::Black;
    for (int i = 0; i < moves; ++i) {
        std::vector<Position> empty;
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (board.isEmpty(r, c)) {
                    empty.push_back({r, c});
                }
            }
        }
        if (empty.empty()) {
            break;
        }
        Position p = empty[rng() % empty.size()];
        board.placePiece(p.row, p.col, color);
        auto last = board.getLastMove();
        if (last.has_value() &&
            board.checkFiveInRow(last->row, last->col, color)) {
            board.undoLastMove();
            break;
        }
        color = (color == ChessPiece::Black) ? ChessPiece::White : ChessPiece::Black;
    }
    return board;
}

bool resultWins(Board board, const SearchResult& result, ChessPiece player) {
    if (!result.valid) {
        return false;
    }
    if (!board.placePiece(result.move.row, result.move.col, player)) {
        return false;
    }
    return board.checkFiveInRow(result.move.row, result.move.col, player);
}

} // namespace

int main() {
    std::mt19937 rng(20260901);
    AIConfig config;
    config.maxDepth = 2;
    config.useTT = false;
    config.useIterativeDeepening = false;

    // Test 1: Legal Move（100 个随机局面）
    {
        SearchEngine engine;
        bool allLegal = true;
        for (int i = 0; i < 100; ++i) {
            Board board = randomBoard(rng, 10);
            ChessPiece player =
                (i % 2 == 0) ? ChessPiece::Black : ChessPiece::White;
            GameStateSnapshot snap = makeSnapshot(board, player);
            SearchResult r = engine.findBestMove(snap, config);
            if (!r.valid ||
                !snap.board.isValidPosition(r.move.row, r.move.col) ||
                !snap.board.isEmpty(r.move.row, r.move.col)) {
                allLegal = false;
                break;
            }
        }
        check(allLegal, "Test 1: Legal Move x100");
    }

    // Test 2: Immediate Win（四个方向）
    {
        SearchEngine engine;
        bool allWin = true;
        const std::vector<std::vector<std::tuple<int, int, ChessPiece>>> cases = {
            {{{7, 7, ChessPiece::Black}, {7, 8, ChessPiece::Black},
              {7, 9, ChessPiece::Black}, {7, 10, ChessPiece::Black}}},
            {{{7, 7, ChessPiece::Black}, {8, 7, ChessPiece::Black},
              {9, 7, ChessPiece::Black}, {10, 7, ChessPiece::Black}}},
            {{{7, 7, ChessPiece::Black}, {8, 8, ChessPiece::Black},
              {9, 9, ChessPiece::Black}, {10, 10, ChessPiece::Black}}},
            {{{7, 7, ChessPiece::Black}, {8, 6, ChessPiece::Black},
              {9, 5, ChessPiece::Black}, {10, 4, ChessPiece::Black}}}
        };
        for (const auto& moves : cases) {
            Board board = makeBoard(moves);
            GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
            SearchResult r = engine.findBestMove(snap, config);
            if (!resultWins(snap.board, r, ChessPiece::Black)) {
                allWin = false;
            }
        }
        check(allWin, "Test 2: Immediate Win x4 directions");
    }

    // Test 3: Immediate Defense（四个方向）
    {
        SearchEngine engine;
        bool allBlock = true;
        const std::vector<std::vector<std::tuple<int, int, ChessPiece>>> cases = {
            {{{7, 7, ChessPiece::White}, {7, 8, ChessPiece::White},
              {7, 9, ChessPiece::White}, {7, 10, ChessPiece::White}}},
            {{{7, 7, ChessPiece::White}, {8, 7, ChessPiece::White},
              {9, 7, ChessPiece::White}, {10, 7, ChessPiece::White}}},
            {{{7, 7, ChessPiece::White}, {8, 8, ChessPiece::White},
              {9, 9, ChessPiece::White}, {10, 10, ChessPiece::White}}},
            {{{7, 7, ChessPiece::White}, {8, 6, ChessPiece::White},
              {9, 5, ChessPiece::White}, {10, 4, ChessPiece::White}}}
        };
        for (const auto& moves : cases) {
            Board board = makeBoard(moves);
            GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
            SearchResult r = engine.findBestMove(snap, config);
            if (!r.valid) {
                allBlock = false;
                continue;
            }
            // 黑方落子后，白方四连必须被破坏
            Board after = snap.board;
            after.placePiece(r.move.row, r.move.col, ChessPiece::Black);
            bool opponentStillThreat = false;
            for (const auto& m : moves) {
                int row = std::get<0>(m);
                int col = std::get<1>(m);
                int count = 1;
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        count = 1;
                        int r2 = row + dr;
                        int c2 = col + dc;
                        while (r2 >= 0 && r2 < BOARD_SIZE &&
                               c2 >= 0 && c2 < BOARD_SIZE &&
                               after.getPiece(r2, c2) == ChessPiece::White) {
                            ++count;
                            r2 += dr;
                            c2 += dc;
                        }
                        r2 = row - dr;
                        c2 = col - dc;
                        while (r2 >= 0 && r2 < BOARD_SIZE &&
                               c2 >= 0 && c2 < BOARD_SIZE &&
                               after.getPiece(r2, c2) == ChessPiece::White) {
                            ++count;
                            r2 -= dr;
                            c2 -= dc;
                        }
                        if (count >= 5) {
                            opponentStillThreat = true;
                        }
                    }
                }
            }
            if (opponentStillThreat) {
                allBlock = false;
            }
        }
        check(allBlock, "Test 3: Immediate Defense x4 directions");
    }

    // Test 4: Terminal（已结束局面不搜索）
    {
        SearchEngine engine;
        Board blackWin = makeBoard({
            {7, 7, ChessPiece::Black}, {7, 8, ChessPiece::Black},
            {7, 9, ChessPiece::Black}, {7, 10, ChessPiece::Black},
            {7, 11, ChessPiece::Black}
        });
        GameStateSnapshot snap1 = makeSnapshot(blackWin, ChessPiece::Black);
        SearchResult r1 = engine.findBestMove(snap1, config);
        check(!r1.valid, "Test 4a: Terminal BlackWin no search");

        Board whiteWin = makeBoard({
            {3, 3, ChessPiece::White}, {3, 4, ChessPiece::White},
            {3, 5, ChessPiece::White}, {3, 6, ChessPiece::White},
            {3, 7, ChessPiece::White}
        });
        GameStateSnapshot snap2 = makeSnapshot(whiteWin, ChessPiece::Black);
        SearchResult r2 = engine.findBestMove(snap2, config);
        check(!r2.valid, "Test 4b: Terminal WhiteWin no search");

        GameStateSnapshot snap3 = makeSnapshot(Board{}, ChessPiece::Black);
        snap3.status = GameState::BlackWin;
        SearchResult r3 = engine.findBestMove(snap3, config);
        check(!r3.valid, "Test 4c: Terminal external status no search");
    }

    // Test 5: Depth 1/2/3 均可安全返回
    {
        SearchEngine engine;
        Board board = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White}
        });
        bool ok = true;
        for (int depth = 1; depth <= 3; ++depth) {
            AIConfig cfg = config;
            cfg.maxDepth = depth;
            GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
            SearchResult r = engine.findBestMove(snap, cfg);
            if (!r.valid ||
                !board.isValidPosition(r.move.row, r.move.col) ||
                !board.isEmpty(r.move.row, r.move.col)) {
                ok = false;
            }
        }
        check(ok, "Test 5: Depth 1/2/3 legal return");
    }

    // Test 6: Repeated Calls x100
    {
        SearchEngine engine;
        bool ok = true;
        for (int i = 0; i < 100; ++i) {
            Board board = randomBoard(rng, 8);
            ChessPiece player =
                (i % 2 == 0) ? ChessPiece::Black : ChessPiece::White;
            GameStateSnapshot snap = makeSnapshot(board, player);
            SearchResult r = engine.findBestMove(snap, config);
            if (!r.valid || !board.isEmpty(r.move.row, r.move.col)) {
                ok = false;
                break;
            }
        }
        check(ok, "Test 6: Repeated Calls x100");
    }


    // Test 7: Timeout（短时限必须安全返回）
    {
        SearchEngine engine;
        AIConfig cfg = config;
        cfg.maxDepth = 4;
        cfg.timeLimitMs = 5;
        cfg.useIterativeDeepening = true;
        Board board = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White},
            {7, 8, ChessPiece::Black}, {8, 9, ChessPiece::White},
            {7, 9, ChessPiece::Black}, {8, 10, ChessPiece::White},
            {7, 10, ChessPiece::Black}
        });
        GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
        auto t0 = std::chrono::steady_clock::now();
        SearchResult r = engine.findBestMove(snap, cfg);
        auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();
        bool ok = elapsed < 5000 && r.valid &&
                  board.isValidPosition(r.move.row, r.move.col) &&
                  board.isEmpty(r.move.row, r.move.col);
        check(ok, "Test 7: Timeout bounded return");
        std::cout << "  requested=" << cfg.timeLimitMs << "ms actual="
                  << elapsed << "ms depth=" << r.completedDepth
                  << " nodes=" << r.nodesVisited << "\n";
    }

    // Test 8: Cancel（搜索中取消必须及时退出）
    {
        SearchEngine engine;
        AIConfig cfg = config;
        cfg.maxDepth = 6;
        cfg.timeLimitMs = 10000;
        cfg.useIterativeDeepening = true;
        Board board = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White},
            {7, 8, ChessPiece::Black}, {8, 9, ChessPiece::White},
            {7, 9, ChessPiece::Black}, {8, 10, ChessPiece::White},
            {7, 10, ChessPiece::Black}, {8, 11, ChessPiece::White}
        });
        GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
        std::atomic<bool> cancelFlag{false};
        std::atomic<bool> started{false};
        SearchResult r;
        std::thread worker([&]() {
            started.store(true);
            r = engine.findBestMove(snap, cfg, &cancelFlag);
        });
        while (!started.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cancelFlag.store(true);
        worker.join();
        check(r.cancelled && r.timeMs < 5000,
              "Test 8: Cancel returns safely and timely");
        std::cout << "  cancel elapsed=" << r.timeMs
                  << "ms depth=" << r.completedDepth
                  << " nodes=" << r.nodesVisited << "\n";
    }

    // Test 9: Node Limit（达到节点上限必须返回合法着法）
    {
        SearchEngine engine;
        AIConfig cfg = config;
        cfg.maxDepth = 4;
        cfg.maxNodes = 200;
        cfg.useIterativeDeepening = true;
        Board board = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White},
            {7, 8, ChessPiece::Black}, {8, 9, ChessPiece::White}
        });
        GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
        SearchResult r = engine.findBestMove(snap, cfg);
        check(r.valid && r.nodeLimitReached &&
              board.isEmpty(r.move.row, r.move.col),
              "Test 9: Node limit returns legal move");
        std::cout << "  nodes=" << r.nodesVisited
                  << " depth=" << r.completedDepth << "\n";
    }

    // Test 10: Iterative Deepening 完整深度
    {
        SearchEngine engine;
        AIConfig cfg = config;
        cfg.maxDepth = 3;
        cfg.timeLimitMs = 0;
        cfg.maxNodes = 0;
        cfg.useIterativeDeepening = true;
        Board board = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White}
        });
        GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
        SearchResult r = engine.findBestMove(snap, cfg);
        check(r.valid && r.completedDepth == 3,
              "Test 10: Iterative Deepening completes depth 3");
        std::cout << "  completedDepth=" << r.completedDepth
                  << " nodes=" << r.nodesVisited << "\n";
    }

    // Test 11: Zobrist determinism
    {
        ZobristHash z1, z2;
        z1.init();
        z2.init();
        Board b1 = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White}
        });
        Board b2 = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White}
        });
        uint64_t h1 = z1.computeHash(b1);
        uint64_t h2 = z2.computeHash(b2);
        check(h1 == h2, "Test 11a: Zobrist same board same hash");
        uint64_t before = h1;
        b1.placePiece(6, 6, ChessPiece::Black);
        uint64_t after = z1.computeHash(b1);
        check(before != after, "Test 11b: Zobrist move changes hash");
        b1.undoLastMove();
        check(z1.computeHash(b1) == before,
              "Test 11c: Zobrist undo restores hash");
    }

    // Test 12: TranspositionTable basic
    {
        TranspositionTable tt;
        tt.store(12345, 2, 500, TTNodeType::Exact, {7, 7});
        int64_t score = 0;
        TTNodeType type = TTNodeType::Exact;
        Position mv{-1, -1};
        bool hit = tt.probe(12345, 2, score, type, mv);
        check(hit && score == 500 && mv.row == 7 && mv.col == 7,
              "Test 12a: TT store/probe");
        tt.clear();
        bool hit2 = tt.probe(12345, 2, score, type, mv);
        check(!hit2, "Test 12b: TT clear");
    }

    // Test 13: TT + Move Ordering 回归
    {
        SearchEngine engine;
        AIConfig cfg = config;
        cfg.maxDepth = 2;
        cfg.useTT = true;
        cfg.useMoveOrdering = true;
        cfg.useIterativeDeepening = false;
        bool legal = true;
        for (int i = 0; i < 50; ++i) {
            Board board = randomBoard(rng, 8);
            GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
            SearchResult r = engine.findBestMove(snap, cfg);
            if (!r.valid || !board.isEmpty(r.move.row, r.move.col)) {
                legal = false;
            }
        }
        Board winBoard = makeBoard({
            {7, 7, ChessPiece::Black}, {7, 8, ChessPiece::Black},
            {7, 9, ChessPiece::Black}, {7, 10, ChessPiece::Black}
        });
        GameStateSnapshot winSnap = makeSnapshot(winBoard, ChessPiece::Black);
        SearchResult winR = engine.findBestMove(winSnap, cfg);
        check(legal && resultWins(winSnap.board, winR, ChessPiece::Black),
              "Test 13: TT+MO regression legal/win");
    }

    // Test 14: 默认配置（TT+ID+MO）回归
    {
        SearchEngine engine;
        AIConfig cfg;
        cfg.maxDepth = 3;
        Board board = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White},
            {7, 8, ChessPiece::Black}, {8, 9, ChessPiece::White}
        });
        GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
        SearchResult r = engine.findBestMove(snap, cfg);
        check(r.valid && r.completedDepth == 3 &&
              board.isEmpty(r.move.row, r.move.col),
              "Test 14: Default config (TT+ID+MO) regression");
        std::cout << "  default nodes=" << r.nodesVisited
                  << " depth=" << r.completedDepth << "\n";
    }

    // Test 15: Draw terminal（满盘且无五连时不再搜索）
    {
        Board full;
        full.init();
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                ChessPiece color =
                    ((r + 2 * c) % 5 <= 1) ? ChessPiece::Black : ChessPiece::White;
                full.placePiece(r, c, color);
            }
        }
        bool noFive = true;
        for (int r = 0; r < BOARD_SIZE && noFive; ++r) {
            for (int c = 0; c < BOARD_SIZE && noFive; ++c) {
                ChessPiece p = full.getPiece(r, c);
                if (p != ChessPiece::Empty && full.checkFiveInRow(r, c, p)) {
                    noFive = false;
                }
            }
        }
        check(noFive, "Test 15a: Draw pattern has no five-in-row");
        GameStateSnapshot snap = makeSnapshot(full, ChessPiece::Black);
        SearchEngine engine;
        AIConfig cfg;
        cfg.maxDepth = 4;
        SearchResult r = engine.findBestMove(snap, cfg);
        check(!r.valid, "Test 15b: Full board terminal no search");
    }

    // Test 16: AIPlayer
    {
        AIConfig cfg = config;
        cfg.maxDepth = 2;
        AIPlayer ai(cfg);
        Board board = makeBoard({
            {7, 7, ChessPiece::Black}, {8, 8, ChessPiece::White}
        });
        GameStateSnapshot snap = makeSnapshot(board, ChessPiece::Black);
        SearchResult r = ai.search(snap);
        check(r.valid && board.isEmpty(r.move.row, r.move.col),
              "Test 16: AIPlayer returns legal move");
    }

    // Test 17: HumanPlayer
    {
        HumanPlayer human;
        human.setMove({7, 7});
        GameStateSnapshot snap;
        Move m = human.getMove(snap);
        check(m.row == 7 && m.col == 7, "Test 17: HumanPlayer roundtrip");
    }

    // Test 18: GameController Human vs Human
    {
        GameController ctrl;
        ctrl.startGame(PlayerType::Human, PlayerType::Human);
        bool noAiTurn = !ctrl.isAITurn();
        std::vector<std::tuple<int, int>> moves = {
            {7, 7}, {8, 8}, {7, 8}, {8, 9}, {7, 9},
            {8, 10}, {7, 10}, {8, 11}, {7, 11}
        };
        for (const auto& m : moves) {
            MoveResult mr = ctrl.applyHumanMove(std::get<0>(m), std::get<1>(m));
            if (mr != MoveResult::Success) noAiTurn = false;
            if (ctrl.isAITurn()) noAiTurn = false;
        }
        check(noAiTurn && ctrl.getState() == GameState::BlackWin,
              "Test 18: GameController HvH win");
    }

    // Test 19: GameController Human vs AI
    {
        GameController ctrl;
        ctrl.startGame(PlayerType::Human, PlayerType::AI);
        bool ok = !ctrl.isAITurn();
        ok = ok && ctrl.applyHumanMove(7, 7) == MoveResult::Success;
        ok = ok && ctrl.isAITurn();
        SearchResult r1 = ctrl.runAIMove();
        ok = ok && r1.valid && !ctrl.isAITurn();
        ok = ok && ctrl.applyHumanMove(8, 7) == MoveResult::Success;
        ok = ok && ctrl.isAITurn();
        SearchResult r2 = ctrl.runAIMove();
        ok = ok && r2.valid && !ctrl.isAITurn();
        ok = ok && ctrl.getState() == GameState::InProgress;
        check(ok, "Test 19: GameController HvA alternating");
    }

    // Test 20: GameController AI first
    {
        GameController ctrl;
        ctrl.startGame(PlayerType::AI, PlayerType::Human);
        bool ok = ctrl.isAITurn();
        SearchResult r = ctrl.runAIMove();
        ok = ok && r.valid && !ctrl.isAITurn();
        ok = ok && ctrl.getState() == GameState::InProgress;
        check(ok, "Test 20: GameController AI first");
    }

    // Test 21: Game 和棋检测
    {
        Game game;
        game.startNewGame();
        auto& board = game.getBoard();
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (r == 0 && c == 0) continue;
                ChessPiece color =
                    ((r + 2 * c) % 5 <= 1) ? ChessPiece::Black : ChessPiece::White;
                board.placePiece(r, c, color);
            }
        }
        MoveResult mr = game.makeMove(0, 0);
        check(mr == MoveResult::Success && game.getState() == GameState::Draw,
              "Test 21: Game draw detection");
    }
    std::cout << "\n========================================\n";




    std::cout << "AI Core + Controller + Final QA Tests\n";
    std::cout << "Passed: " << g_passed << "\n";
    std::cout << "Failed: " << g_failed << "\n";
    std::cout << "========================================\n";

    return g_failed == 0 ? 0 : 1;
}
