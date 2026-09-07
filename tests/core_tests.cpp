#include "GomokuCore.h"

#include <iostream>

using namespace Gomoku;

#define CHECK(x) do { if (!(x)) { \
    std::cerr << "FAIL: " #x << std::endl; return 1; } } while (0)

int main() {
    GameEngine game;
    // Black occupies row 7 columns 3-6; white blocks are placed far away.
    game.makeMove(7, 3);
    game.makeMove(0, 0);
    game.makeMove(7, 4);
    game.makeMove(0, 1);
    game.makeMove(7, 5);
    game.makeMove(0, 2);
    game.makeMove(7, 6);
    game.makeMove(1, 0);
    CHECK(game.status() == GameStatus::InProgress);
    CHECK(game.isFiveAt(7, 3, Piece::Black) == false);

    AiConfig easy{ 3, 800, 100000 };
    AiResult best = AiSearch::findBestMove(game, Piece::Black, easy);
    std::cout << "AI move " << best.move.row << "," << best.move.col
              << " depth=" << best.completedDepth
              << " nodes=" << best.nodesVisited << "\n";
    for (const GameMove& move : game.history()) {
        std::cout << "hist " << (move.piece == Piece::Black ? "B" : "W")
                  << " " << move.row << "," << move.col << "\n";
    }
    CHECK(best.valid);
    CHECK((best.move.row == 7 && (best.move.col == 2 || best.move.col == 7)));

    // Five in a row ends the game.
    GameEngine winGame;
    winGame.makeMove(3, 3);
    winGame.makeMove(14, 14);
    winGame.makeMove(4, 4);
    winGame.makeMove(13, 13);
    winGame.makeMove(5, 5);
    winGame.makeMove(12, 12);
    winGame.makeMove(6, 6);
    winGame.makeMove(11, 11);
    winGame.makeMove(7, 7);
    CHECK(winGame.status() == GameStatus::BlackWin);
    CHECK(!winGame.winningLine(7, 7).empty());

    winGame.undo();
    CHECK(winGame.status() == GameStatus::InProgress);
    CHECK(winGame.currentPlayer() == Piece::Black);

    std::cout << "gomoku core ok: ai-win-move, win detect, undo\n";
    return 0;
}
