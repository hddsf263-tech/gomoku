#include "BoardWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QRandomGenerator>

#include <algorithm>
#include <cmath>

namespace Gomoku {

namespace {

constexpr int kSide = 15;
constexpr int kLeft = 28;
constexpr int kTop = 24;
constexpr int kRight = 12;
constexpr int kBottom = 10;

QColor alphaColor(const QColor& color, int alpha) {
    QColor result = color;
    result.setAlpha(alpha);
    return result;
}

QPointF pointFromProgress(const QRect& rect, double x, double y) {
    return QPointF(rect.left() + rect.width() * x,
                   rect.top() + rect.height() * y);
}

} // namespace

BoardWidget::BoardWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(380, 360);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ringTimer_.setInterval(24);
    connect(&ringTimer_, &QTimer::timeout, this, [this]() {
        if (ringProgress_ >= 0) {
            ringProgress_ += 0.10;
            if (ringProgress_ >= 1.0) {
                ringProgress_ = -1.0;
                ringTimer_.stop();
            }
        }
        update();
    });

    particleTimer_.setInterval(30);
    connect(&particleTimer_, &QTimer::timeout, this, [this]() {
        bool alive = false;
        for (Particle& particle : particles_) {
            particle.age += 0.03;
            particle.x += particle.vx * 0.03;
            particle.y += particle.vy * 0.03;
            particle.vy += 140.0 * 0.03;
            if (particle.age < particle.life) {
                alive = true;
            }
        }
        particles_.erase(
            std::remove_if(particles_.begin(), particles_.end(),
                           [](const Particle& p) {
                return p.age >= p.life;
            }),
            particles_.end());
        if (!alive) {
            particleTimer_.stop();
        }
        update();
    });

    winTimer_.setInterval(430);
    connect(&winTimer_, &QTimer::timeout, this, [this]() {
        winPulse_ = !winPulse_;
        update();
    });
}

void BoardWidget::setBoardColors(QColor base, QColor line, QColor star) {
    boardBase_ = base;
    lineColor_ = line;
    starColor_ = star;
    update();
}

void BoardWidget::setBoardImage(const QString& path) {
    boardImage_ = QPixmap();
    if (!path.isEmpty()) {
        boardImage_.load(path);
        boardImage_ = boardImage_.scaled(
            800, 800, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
    update();
}

void BoardWidget::setPieceImages(const QString& blackPath, const QString& whitePath) {
    blackImage_ = QPixmap();
    whiteImage_ = QPixmap();
    blackImage_.load(blackPath);
    whiteImage_.load(whitePath);
    if (!blackImage_.isNull()) {
        blackImage_ = blackImage_.scaled(
            220, 220, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
    if (!whiteImage_.isNull()) {
        whiteImage_ = whiteImage_.scaled(
            220, 220, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
    update();
}

void BoardWidget::setPieceGradient(int piece, const QColor& light, const QColor& dark) {
    if (piece == 0) {
        blackLight_ = light;
        blackDark_ = dark;
    } else {
        whiteLight_ = light;
        whiteDark_ = dark;
    }
    update();
}

void BoardWidget::clearGameVisuals() {
    ringProgress_ = -1.0;
    ringTimer_.stop();
    particles_.clear();
    particleTimer_.stop();
    winLine_.clear();
    winTimer_.stop();
    hoverRow_ = -1;
    hoverCol_ = -1;
    update();
}

int BoardWidget::cellSize() const {
    const QRect area = gridArea();
    return qMax(16, qMin(area.width(), area.height()) / (kSide - 1));
}

QRect BoardWidget::gridArea() const {
    const int areaWidth = qMax(100, width() - kLeft - kRight);
    const int areaHeight = qMax(100, height() - kTop - kBottom);
    const int cell = qMin(areaWidth, areaHeight) / (kSide - 1);
    const int side = cell * (kSide - 1);
    return QRect(kLeft + (areaWidth - side) / 2,
                 kTop + (areaHeight - side) / 2,
                 side,
                 side);
}

QPoint BoardWidget::gridToScreen(int row, int col) const {
    const QRect area = gridArea();
    const int cell = area.width() / (kSide - 1);
    return QPoint(area.left() + col * cell, area.top() + row * cell);
}

void BoardWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);
    drawGrid(painter);
    drawCoordinates(painter);
    if (game_) {
        drawGhost(painter);
        drawStones(painter);
    }
    drawParticles(painter);
    drawRing(painter);
    drawWinLine(painter);
}

void BoardWidget::drawBackground(QPainter& painter) {
    painter.fillRect(rect(), QColor(242, 241, 236));
    const QRect area = gridArea();
    const QRect boardRect = area.adjusted(-10, -10, 10, 10);

    QPainterPath path;
    path.addRoundedRect(boardRect, 12, 12);
    if (!boardImage_.isNull()) {
        painter.save();
        painter.setClipPath(path);
        painter.drawPixmap(boardRect, boardImage_);
        painter.setBrush(QColor(14, 26, 22, 55));
        painter.setPen(Qt::NoPen);
        painter.drawRect(boardRect);
        painter.restore();
    } else {
        QLinearGradient gradient(boardRect.topLeft(), boardRect.bottomRight());
        gradient.setColorAt(0, boardBase_.lighter(108));
        gradient.setColorAt(1, boardBase_.darker(112));
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawPath(path);
    }

    painter.setPen(QPen(QColor(0, 0, 0, 45), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void BoardWidget::drawGrid(QPainter& painter) {
    const QRect area = gridArea();
    const int cell = area.width() / (kSide - 1);

    QPen gridPen(lineColor_, 1);
    painter.setPen(gridPen);
    for (int i = 0; i < kSide; i++) {
        const int pos = area.left() + i * cell;
        painter.drawLine(pos, area.top(), pos, area.bottom());
        painter.drawLine(area.left(), pos, area.right(), pos);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(starColor_);
    static const int stars[5][2] = { {3, 3}, {3, 11}, {7, 7}, {11, 3}, {11, 11} };
    for (const auto& star : stars) {
        const QPoint p = gridToScreen(star[0], star[1]);
        painter.drawEllipse(p, 3, 3);
    }
}

void BoardWidget::drawCoordinates(QPainter& painter) {
    const QRect area = gridArea();
    const int cell = area.width() / (kSide - 1);
    QFont font = painter.font();
    font.setPixelSize(10);
    font.setWeight(QFont::DemiBold);
    painter.setFont(font);
    painter.setPen(QColor(107, 116, 111));

    for (int i = 0; i < kSide; i++) {
        const QPoint top = gridToScreen(0, i);
        const QPoint left = gridToScreen(i, 0);
        painter.drawText(QRect(top.x() - 40, 2, 80, 18),
                         Qt::AlignCenter, QString(QChar('A' + i)));
        painter.drawText(QRect(0, left.y() - 8, kLeft - 4, 16),
                         Qt::AlignRight, QString::number(i + 1));
    }
}

void BoardWidget::drawGhost(QPainter& painter) {
    if (!ghostAllowed_ || thinking_ || hoverRow_ < 0 || !game_ ||
        game_->status() != GameStatus::InProgress ||
        game_->pieceAt(hoverRow_, hoverCol_) != Piece::Empty) {
        return;
    }

    const QPoint center = gridToScreen(hoverRow_, hoverCol_);
    const int cell = gridArea().width() / (kSide - 1);
    const double radius = cell * 0.42;
    const Piece piece = game_->currentPlayer();
    const bool isBlack = piece == Piece::Black;
    QColor fill = isBlack ? QColor(24, 28, 30, 110) : QColor(255, 255, 255, 120);
    QPen pen(isBlack ? QColor(255, 255, 255, 90) : QColor(40, 48, 46, 70), 1.5);
    painter.setPen(pen);
    painter.setBrush(fill);
    painter.drawEllipse(center, static_cast<int>(radius), static_cast<int>(radius));
}

void BoardWidget::drawStones(QPainter& painter) {
    const int cell = gridArea().width() / (kSide - 1);
    const double radius = cell * 0.43;

    for (int row = 0; row < kSide; row++) {
        for (int col = 0; col < kSide; col++) {
            const Piece piece = game_->pieceAt(row, col);
            if (piece == Piece::Empty) {
                continue;
            }
            const QPoint center = gridToScreen(row, col);

            QPainterPath shadowPath;
            shadowPath.addEllipse(QPointF(center.x() + 2, center.y() + 2),
                                  radius, radius);
            painter.fillPath(shadowPath, QColor(0, 0, 0, 70));

            const QRectF circle(center.x() - radius,
                                center.y() - radius,
                                radius * 2,
                                radius * 2);
            const bool isBlack = piece == Piece::Black;
            QPixmap* image = isBlack ? &blackImage_ : &whiteImage_;
            QColor light = isBlack ? blackLight_ : whiteLight_;
            QColor dark = isBlack ? blackDark_ : whiteDark_;

            QPainterPath stonePath;
            stonePath.addEllipse(circle);
            painter.save();
            painter.setClipPath(stonePath);
            if (!image->isNull()) {
                painter.drawPixmap(circle.toRect(), *image);
                painter.fillRect(circle, QColor(0, 0, 0, 18));
            } else {
                QRadialGradient gradient(center.x() - radius * 0.3,
                                         center.y() - radius * 0.35,
                                         radius * 1.25);
                gradient.setColorAt(0, light);
                gradient.setColorAt(1, dark);
                painter.setPen(Qt::NoPen);
                painter.setBrush(gradient);
                painter.drawEllipse(circle);
            }
            QRadialGradient gloss(center.x() - radius * 0.45,
                                  center.y() - radius * 0.5,
                                  radius * 0.7);
            gloss.setColorAt(0, QColor(255, 255, 255, 70));
            gloss.setColorAt(1, QColor(255, 255, 255, 0));
            painter.fillPath(stonePath, gloss);
            painter.restore();

            if (isBlack) {
                painter.setPen(QPen(QColor(0, 0, 0, 120), 1));
            } else {
                painter.setPen(QPen(QColor(90, 90, 90, 90), 1));
            }
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(circle);
        }
    }
}

void BoardWidget::drawRing(QPainter& painter) {
    if (ringProgress_ < 0 || ringRow_ < 0) {
        return;
    }
    const int cell = gridArea().width() / (kSide - 1);
    const QPoint center = gridToScreen(ringRow_, ringCol_);
    const double progress = ringProgress_;
    const double radius = cell * (0.45 + progress * 0.75);
    const Piece piece = game_ ? game_->pieceAt(ringRow_, ringCol_) : Piece::Empty;
    const QColor color = piece == Piece::Black
        ? QColor(255, 255, 255, 230)
        : QColor(30, 40, 38, 220);
    QPen pen(color, 2);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, static_cast<int>(radius),
                        static_cast<int>(radius));
}

void BoardWidget::playPlaceEffect(int row, int col, Piece piece) {
    if (placeEffect_ == "none") {
        return;
    }
    static const std::vector<QColor> sparkColors = {
        QColor(255, 255, 255),
        QColor(255, 215, 106),
        QColor(159, 232, 255),
        QColor(255, 157, 157)
    };
    if (placeEffect_ == "spark") {
        spawnParticles(row, col, 12, 1.15, sparkColors);
    } else {
        ringRow_ = row;
        ringCol_ = col;
        ringProgress_ = 0.0;
        ringTimer_.start();
    }
    update();
}

void BoardWidget::playWinEffect(const std::vector<GameMove>& line) {
    winLine_ = line;
    winPulse_ = true;
    if (winEffect_ != "none") {
        winTimer_.start();
    }

    static const std::vector<QColor> burstColors = {
        QColor(255, 215, 106),
        QColor(255, 157, 157),
        QColor(159, 232, 255),
        QColor(183, 240, 165),
        QColor(255, 247, 214)
    };
    if (winEffect_ == "burst" && !line.empty()) {
        spawnParticles(line[line.size() / 2].row,
                       line[line.size() / 2].col,
                       30, 2.6, burstColors);
    }
}

void BoardWidget::drawWinLine(QPainter& painter) {
    if (winLine_.empty()) {
        return;
    }
    const int cell = gridArea().width() / (kSide - 1);
    const QColor accent = winPulse_
        ? QColor(224, 166, 62, 250)
        : QColor(224, 166, 62, 160);
    QPen pen(accent, qMax(3, cell / 14));
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    for (const GameMove& move : winLine_) {
        const QPoint p = gridToScreen(move.row, move.col);
        painter.drawEllipse(p, static_cast<int>(cell * 0.48),
                            static_cast<int>(cell * 0.48));
    }
}

void BoardWidget::drawParticles(QPainter& painter) {
    for (const Particle& particle : particles_) {
        const double alpha = 1.0 - particle.age / particle.life;
        QColor color = particle.color;
        color.setAlphaF(qMax(0.0, alpha));
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        const double size = particle.size * (1.0 - 0.35 * alpha);
        painter.drawEllipse(QPointF(particle.x, particle.y),
                            size, size);
    }
}

void BoardWidget::spawnParticles(int row, int col, int count,
                                 double spread,
                                 const std::vector<QColor>& colors) {
    const QPoint center = gridToScreen(row, col);
    const double cell = gridArea().width() / (kSide - 1);
    for (int i = 0; i < count; i++) {
        const double angle = 2.0 * M_PI * i / count +
                             (QRandomGenerator::global()->generateDouble() - 0.5) * 0.7;
        const double speed = cell * (0.7 + QRandomGenerator::global()->generateDouble() * 0.9) * spread;
        Particle particle;
        particle.x = center.x();
        particle.y = center.y();
        particle.vx = std::cos(angle) * speed;
        particle.vy = std::sin(angle) * speed - 25.0;
        particle.life = 0.65 + QRandomGenerator::global()->generateDouble() * 0.25;
        particle.size = qMax(2.5, cell * 0.10);
        particle.color = colors[i % colors.size()];
        particles_.push_back(particle);
    }
    particleTimer_.start();
}

void BoardWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && game_) {
        const QPoint p = event->pos();
        const int cell = gridArea().width() / (kSide - 1);
        int col = qRound((p.x() - gridArea().left()) / static_cast<double>(cell));
        int row = qRound((p.y() - gridArea().top()) / static_cast<double>(cell));
        if (row >= 0 && row < kSide && col >= 0 && col < kSide) {
            emit positionClicked(row, col);
        }
    }
}

void BoardWidget::mouseMoveEvent(QMouseEvent* event) {
    const int cell = gridArea().width() / (kSide - 1);
    hoverCol_ = qRound((event->pos().x() - gridArea().left()) /
                       static_cast<double>(cell));
    hoverRow_ = qRound((event->pos().y() - gridArea().top()) /
                       static_cast<double>(cell));
    if (hoverRow_ < 0 || hoverRow_ >= kSide ||
        hoverCol_ < 0 || hoverCol_ >= kSide) {
        hoverRow_ = -1;
        hoverCol_ = -1;
    }
    update();
}

void BoardWidget::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    hoverRow_ = -1;
    hoverCol_ = -1;
    update();
}

} // namespace Gomoku
