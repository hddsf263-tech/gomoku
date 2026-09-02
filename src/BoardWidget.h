#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include "../include/Board.h"
#include "../include/Game.h"

// Author: [组员姓名待填写]
// Module: BoardWidget
// Description: 棋盘可视化控件

class BoardWidget : public QWidget {
    Q_OBJECT

public:
    explicit BoardWidget(QWidget *parent = nullptr);
    ~BoardWidget() override = default;

    /// @brief 设置游戏对象指针
    void setGame(Gomoku::Game* game);

    /// @brief 更新棋盘显示
    void updateBoard();

signals:
    /// @brief 玩家点击位置信号
    void positionClicked(int row, int col);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    /// @brief 绘制棋盘网格
    void drawGrid(QPainter& painter);
    
    /// @brief 绘制棋子
    void drawPieces(QPainter& painter);
    
    /// @brief 绘制最后一步标记
    void drawLastMoveMarker(QPainter& painter);
    
    /// @brief 将鼠标位置转换为棋盘坐标
    std::pair<int, int> posToGrid(int x, int y) const;
    
    /// @brief 将棋盘坐标转换为像素位置
    std::pair<int, int> gridToPos(int row, int col) const;

    Gomoku::Game* game;                      ///< 游戏对象指针
    int cellSize;                            ///< 格子大小
    int margin;                              ///< 边距
    int pieceRadius;                         ///< 棋子半径
};

#endif // BOARDWIDGET_H
