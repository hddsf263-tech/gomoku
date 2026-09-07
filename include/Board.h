#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <optional>
#include "ChessPiece.h"

// Author: [组员姓名待填写]
// Module: Board
// Description: 五子棋棋盘数据与落子管理

namespace Gomoku {

constexpr int BOARD_SIZE = 15;  ///< 标准棋盘大小

/// @brief 棋盘位置结构
struct Position {
    int row;
    int col;
    int score = 0;
    
    bool isValid() const { return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE; }
    bool operator==(const Position& other) const { return row == other.row && col == other.col; }
};

/// @brief 棋盘类 - 负责棋盘状态管理
class Board {
public:
    static constexpr int SIZE = BOARD_SIZE;  ///< 标准 15x15 棋盘
    
    Board();
    ~Board() = default;
    
    // 允许拷贝（用于 AI 搜索）
    
    /// @brief 初始化棋盘
    void init();
    
    /// @brief 清空棋盘
    void clear();
    
    /// @brief 判断位置是否合法
    bool isValidPosition(int row, int col) const;
    
    /// @brief 判断位置是否为空
    bool isEmpty(int row, int col) const;
    
    /// @brief 放置棋子
    /// @return 成功返回 true，失败返回 false
    bool placePiece(int row, int col, ChessPiece piece);
    
    /// @brief 查询指定位置的棋子
    ChessPiece getPiece(int row, int col) const;
    
    /// @brief 获取最后一步落子位置
    std::optional<Position> getLastMove() const { return lastMove; }
    
    /// @brief 撤销最后一步
    /// @return 成功返回 true，失败返回 false
    bool undoLastMove();
    
    /// @brief 获取所有落子历史
    const std::vector<Position>& getMoveHistory() const { return moveHistory; }
    
    /// @brief 检查指定位置是否形成五连
    /// @return 如果形成五连返回 true
    bool checkFiveInRow(int row, int col, ChessPiece piece) const;
    
    /// @brief 获取指定位置所在的获胜连线
    /// @return 获胜连线上的所有位置；若未形成五连则返回空
    std::vector<Position> getWinningLine(int row, int col, ChessPiece piece) const;
    
private:
    /// @brief 检查某个方向的连续棋子数
    int countConsecutive(int row, int col, int deltaRow, int deltaCol, ChessPiece piece) const;
    
    std::vector<std::vector<ChessPiece>> grid;  ///< 棋盘网格
    std::vector<Position> moveHistory;           ///< 落子历史
    std::optional<Position> lastMove;            ///< 最后一步位置
};

} // namespace Gomoku

#endif // BOARD_H
