#pragma once

#include <QColor>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <vector>

#include "GomokuCore.h"

namespace Gomoku {

class BoardWidget : public QWidget {
    Q_OBJECT

public:
    explicit BoardWidget(QWidget* parent = nullptr);

    void setGame(GameEngine* game) { game_ = game; }

    void setBoardColors(QColor base, QColor line, QColor star);
    void setBoardImage(const QString& path);
    void setPieceImages(const QString& blackPath, const QString& whitePath);
    void setPieceGradient(int piece, const QColor& light, const QColor& dark);

    void setGhostAllowed(bool allowed) { ghostAllowed_ = allowed; }
    void setThinking(bool thinking) { thinking_ = thinking; update(); }
    void setEffectModes(const QString& placeEffect, const QString& winEffect) {
        placeEffect_ = placeEffect;
        winEffect_ = winEffect;
    }
    void clearGameVisuals();

    void playPlaceEffect(int row, int col, Piece piece);
    void playWinEffect(const std::vector<GameMove>& line);

signals:
    void positionClicked(int row, int col);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Particle {
        double x = 0;
        double y = 0;
        double vx = 0;
        double vy = 0;
        double age = 0;
        double life = 1;
        double size = 3;
        QColor color;
    };

    QRect boardRect() const;
    QRect gridArea() const;
    int cellSize() const;
    QPointF gridToScreenF(int row, int col) const;
    QPoint gridToScreen(int row, int col) const;

    void drawBackground(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawCoordinates(QPainter& painter);
    void drawGhost(QPainter& painter);
    void drawStones(QPainter& painter);
    void drawParticles(QPainter& painter);
    void drawRing(QPainter& painter);
    void drawWinLine(QPainter& painter);

    void spawnParticles(int row, int col, int count,
                        double spread, const std::vector<QColor>& colors);

    GameEngine* game_ = nullptr;

    QColor boardBase_ = QColor(46, 93, 82);
    QColor lineColor_ = QColor(213, 228, 218);
    QColor starColor_ = QColor(194, 216, 203);
    QPixmap boardImage_;
    QPixmap blackImage_;
    QPixmap whiteImage_;
    QColor blackLight_ = QColor(78, 86, 85);
    QColor blackDark_ = QColor(16, 18, 20);
    QColor whiteLight_ = QColor(255, 255, 255);
    QColor whiteDark_ = QColor(221, 217, 203);
    QString placeEffect_ = "ring";
    QString winEffect_ = "pulse";

    bool ghostAllowed_ = true;
    bool thinking_ = false;
    int hoverRow_ = -1;
    int hoverCol_ = -1;

    double ringProgress_ = -1.0;
    int ringRow_ = -1;
    int ringCol_ = -1;
    QTimer ringTimer_;

    QTimer particleTimer_;
    std::vector<Particle> particles_;

    std::vector<GameMove> winLine_;
    QTimer winTimer_;
    bool winPulse_ = true;
};

} // namespace Gomoku
