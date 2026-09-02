#include "BoardWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <cmath>

// Author: [组员姓名待填写]
// Module: BoardWidget Implementation
// Description: 棋盘可视化控件实现

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent)
    , game(nullptr)
    , cellSize(30)
    , margin(20)
    , pieceRadius(13)
{
    setMinimumSize(400, 400);
    setMouseTracking(true);
}

void BoardWidget::setGame(Gomoku::Game* gamePtr) {
    game = gamePtr;
}

void BoardWidget::updateBoard() {
    update();
}

std::pair<int, int> BoardWidget::posToGrid(int x, int y) const {
    // 找到最近的交叉点
    int row = static_cast<int>(std::round((y - margin) / static_cast<double>(cellSize)));
    int col = static_cast<int>(std::round((x - margin) / static_cast<double>(cellSize)));
    return {row, col};
}

std::pair<int, int> BoardWidget::gridToPos(int row, int col) const {
    int x = margin + col * cellSize;
    int y = margin + row * cellSize;
    return {x, y};
}

void BoardWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    painter.fillRect(rect(), QColor(220, 179, 92));  // 木质颜色
    
    // 绘制棋盘网格
    drawGrid(painter);
    
    // 绘制棋子
    if (game) {
        drawPieces(painter);
        drawLastMoveMarker(painter);
    }
}

void BoardWidget::drawGrid(QPainter& painter) {
    QPen pen(QColor(60, 40, 20), 1);
    painter.setPen(pen);
    
    int boardPixelSize = (Gomoku::Board::SIZE - 1) * cellSize;
    
    // 绘制横线和竖线
    for (int i = 0; i < Gomoku::Board::SIZE; ++i) {
        int pos = margin + i * cellSize;
        
        // 横线
        painter.drawLine(margin, pos, margin + boardPixelSize, pos);
        // 竖线
        painter.drawLine(pos, margin, pos, margin + boardPixelSize);
    }
    
    // 绘制星位点（标准五子棋棋盘上的 5 个点）
    static const int starPoints[5][2] = {
        {3, 3}, {3, 11}, {11, 3}, {11, 11}, {7, 7}
    };
    
    painter.setBrush(QColor(60, 40, 20));
    for (const auto& point : starPoints) {
        auto [x, y] = gridToPos(point[0], point[1]);
        painter.drawEllipse(x - 3, y - 3, 6, 6);
    }
}

void BoardWidget::drawPieces(QPainter& painter) {
    const auto& board = game->getBoard();
    
    for (int row = 0; row < Gomoku::Board::SIZE; ++row) {
        for (int col = 0; col < Gomoku::Board::SIZE; ++col) {
            auto piece = board.getPiece(row, col);
            if (piece != Gomoku::ChessPiece::Empty) {
                auto [x, y] = gridToPos(row, col);
                
                // 绘制棋子阴影
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 0, 0, 80));
                painter.drawEllipse(x - pieceRadius + 2, y - pieceRadius + 2, 
                                   pieceRadius * 2, pieceRadius * 2);
                
                // 绘制棋子
                if (piece == Gomoku::ChessPiece::Black) {
                    QRadialGradient gradient(x - 3, y - 3, pieceRadius * 2);
                    gradient.setColorAt(0, QColor(80, 80, 80));
                    gradient.setColorAt(1, QColor(0, 0, 0));
                    painter.setBrush(gradient);
                } else {
                    QRadialGradient gradient(x - 3, y - 3, pieceRadius * 2);
                    gradient.setColorAt(0, QColor(255, 255, 255));
                    gradient.setColorAt(1, QColor(200, 200, 200));
                    painter.setBrush(gradient);
                }
                
                painter.drawEllipse(x - pieceRadius, y - pieceRadius, 
                                   pieceRadius * 2, pieceRadius * 2);
            }
        }
    }
}

void BoardWidget::drawLastMoveMarker(QPainter& painter) {
    const auto& board = game->getBoard();
    auto lastMove = board.getLastMove();
    
    if (lastMove.has_value()) {
        auto [x, y] = gridToPos(lastMove->row, lastMove->col);
        
        // 绘制红色标记
        painter.setPen(QPen(QColor(255, 0, 0), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(x - 4, y - 4, 8, 8);
    }
}

void BoardWidget::mousePressEvent(QMouseEvent *event) {
    if (!game || event->button() != Qt::LeftButton) {
        return;
    }
    
    auto [row, col] = posToGrid(event->pos().x(), event->pos().y());
    
    const auto& board = game->getBoard();
    if (board.isValidPosition(row, col)) {
        emit positionClicked(row, col);
    }
}

void BoardWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    // 根据控件大小动态调整格子大小
    int minDimension = qMin(width(), height());
    cellSize = (minDimension - 2 * margin) / (Gomoku::Board::SIZE - 1);
    pieceRadius = cellSize / 3;
}
