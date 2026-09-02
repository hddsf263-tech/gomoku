#ifndef CHESSPIECE_H
#define CHESSPIECE_H

// Author: [组员姓名待填写]
// Module: ChessPiece
// Description: 棋子枚举定义

namespace Gomoku {

/// @brief 棋子类型枚举
enum class ChessPiece {
    Empty = 0,    ///< 空位置
    Black = 1,    ///< 黑棋
    White = 2     ///< 白棋
};

/// @brief 游戏状态枚举
enum class GameState {
    NotStarted,      ///< 游戏未开始
    InProgress,      ///< 游戏进行中
    BlackWin,        ///< 黑方胜利
    WhiteWin,        ///< 白方胜利
    Draw             ///< 平局
};

/// @brief 玩家类型枚举
enum class PlayerType {
    Human,           ///< 人类玩家
    AI               ///< AI 玩家
};

} // namespace Gomoku

#endif // CHESSPIECE_H
