#include "Board.h"
#include <algorithm>

// Author: [组员姓名待填写]
// Module: Board Implementation
// Description: 棋盘类实现

namespace Gomoku {

Board::Board() : grid(BOARD_SIZE, std::vector<ChessPiece>(BOARD_SIZE, ChessPiece::Empty)) {
}

void Board::init() {
    clear();
}

void Board::clear() {
    for (auto& row : grid) {
        std::fill(row.begin(), row.end(), ChessPiece::Empty);
    }
    moveHistory.clear();
    lastMove = std::nullopt;
}

bool Board::isValidPosition(int row, int col) const {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

bool Board::isEmpty(int row, int col) const {
    if (!isValidPosition(row, col)) {
        return false;
    }
    return grid[row][col] == ChessPiece::Empty;
}

bool Board::placePiece(int row, int col, ChessPiece piece) {
    if (!isValidPosition(row, col)) {
        return false;
    }
    if (!isEmpty(row, col)) {
        return false;
    }
    
    grid[row][col] = piece;
    moveHistory.push_back({row, col});
    lastMove = {row, col};
    return true;
}

ChessPiece Board::getPiece(int row, int col) const {
    if (!isValidPosition(row, col)) {
        return ChessPiece::Empty;
    }
    return grid[row][col];
}

bool Board::undoLastMove() {
    if (moveHistory.empty() || !lastMove.has_value()) {
        return false;
    }
    
    Position pos = lastMove.value();
    grid[pos.row][pos.col] = ChessPiece::Empty;
    moveHistory.pop_back();
    
    if (!moveHistory.empty()) {
        lastMove = moveHistory.back();
    } else {
        lastMove = std::nullopt;
    }
    
    return true;
}

bool Board::checkFiveInRow(int row, int col, ChessPiece piece) const {
    if (piece == ChessPiece::Empty) {
        return false;
    }
    
    // 四个方向：横向、纵向、左上 - 右下、右上 - 左下
    static const int directions[4][2] = {
        {0, 1},   // 横向
        {1, 0},   // 纵向
        {1, 1},   // 左上到右下
        {1, -1}   // 右上到左下
    };
    
    for (const auto& dir : directions) {
        int count = countConsecutive(row, col, dir[0], dir[1], piece);
        if (count >= 5) {
            return true;
        }
    }
    
    return false;
}

int Board::countConsecutive(int row, int col, int deltaRow, int deltaCol, ChessPiece piece) const {
    int count = 1;  // 包含当前棋子
    
    // 正方向计数
    int r = row + deltaRow;
    int c = col + deltaCol;
    while (isValidPosition(r, c) && grid[r][c] == piece) {
        count++;
        r += deltaRow;
        c += deltaCol;
    }
    
    // 反方向计数
    r = row - deltaRow;
    c = col - deltaCol;
    while (isValidPosition(r, c) && grid[r][c] == piece) {
        count++;
        r -= deltaRow;
        c -= deltaCol;
    }
    
    return count;
}

} // namespace Gomoku
