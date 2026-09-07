#include "BoardWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QRandomGenerator>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent)
    , game(nullptr)
    , cellSize(30)
    , margin(20)
    , pieceRadius(13)
{
    setMinimumSize(400, 400);
    setMouseTracking(true);
    m_clock.start();
    m_effectTimer.setInterval(30);
    connect(&m_effectTimer, &QTimer::timeout, this, [this]() {
        onEffectTick();
    });
}

void BoardWidget::setGame(Gomoku::Game* gamePtr) {
    game = gamePtr;
}

void BoardWidget::configure(const AppCfg::AppSettings& settings) {
    m_settings = settings;
    update();
}

void BoardWidget::updateBoard() {
    update();
}

std::pair<int, int> BoardWidget::posToGrid(int x, int y) const {
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
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 棋盘抖动偏移
    if (m_winShake && m_clock.isValid()) {
        double t = static_cast<double>(m_clock.elapsed() - m_shakeStartMs);
        if (t < m_shakeDurationMs) {
            double k = 1.0 - t / m_shakeDurationMs;
            double amp = 6.0 * k;
            painter.translate(std::sin(t * 0.09) * amp,
                              std::cos(t * 0.09) * amp);
        } else {
            m_winShake = false;
        }
    }

    drawBoardBackground(painter);
    drawGrid(painter);

    if (game) {
        drawPieces(painter);
        drawLastMoveMarker(painter);
        drawWinHighlight(painter);
    }

    drawEffects(painter);
}

void BoardWidget::drawBoardBackground(QPainter& painter) {
    painter.fillRect(rect(), AppCfg::boardBase(m_settings.boardSkin));

    // 若启用自定义背景图片，覆盖绘制
    if (m_settings.boardSkin == AppCfg::BoardSkin::Custom &&
        !m_settings.boardImagePath.isEmpty()) {
        QImage image = loadImageCached(m_settings.boardImagePath);
        if (!image.isNull()) {
            QImage scaled = image.scaled(size(),
                                         Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation);
            int x = (scaled.width() - width()) / 2;
            int y = (scaled.height() - height()) / 2;
            painter.drawImage(x, y, scaled);
        }
    } else {
        // 轻微渐变叠加，增加质感
        QLinearGradient overlay(rect().topLeft(), rect().bottomRight());
        overlay.setColorAt(0, QColor(255, 255, 255, 14));
        overlay.setColorAt(1, QColor(0, 0, 0, 30));
        painter.fillRect(rect(), overlay);
    }
}

void BoardWidget::drawGrid(QPainter& painter) {
    QPen pen(AppCfg::boardLine(m_settings.boardSkin), 1);
    painter.setPen(pen);

    int boardPixelSize = (Gomoku::Board::SIZE - 1) * cellSize;

    for (int i = 0; i < Gomoku::Board::SIZE; ++i) {
        int pos = margin + i * cellSize;
        painter.drawLine(margin, pos, margin + boardPixelSize, pos);
        painter.drawLine(pos, margin, pos, margin + boardPixelSize);
    }

    static const int starPoints[5][2] = {
        {3, 3}, {3, 11}, {11, 3}, {11, 11}, {7, 7}
    };

    painter.setBrush(AppCfg::boardStar(m_settings.boardSkin));
    for (const auto& point : starPoints) {
        auto [x, y] = gridToPos(point[0], point[1]);
        painter.drawEllipse(x - 3, y - 3, 6, 6);
    }
}

void BoardWidget::drawPieces(QPainter& painter) {
    const auto& board = game->getBoard();
    const AppCfg::PieceSkin skin = m_settings.pieceSkin;

    for (int row = 0; row < Gomoku::Board::SIZE; ++row) {
        for (int col = 0; col < Gomoku::Board::SIZE; ++col) {
            auto piece = board.getPiece(row, col);
            if (piece == Gomoku::ChessPiece::Empty) {
                continue;
            }

            auto [x, y] = gridToPos(row, col);

            // 阴影
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 80));
            painter.drawEllipse(x - pieceRadius + 2, y - pieceRadius + 2,
                                pieceRadius * 2, pieceRadius * 2);

            QRectF circleRect(x - pieceRadius, y - pieceRadius,
                              pieceRadius * 2, pieceRadius * 2);

            const bool isBlack = (piece == Gomoku::ChessPiece::Black);

            // 自定义图片棋子
            QString imagePath = isBlack ? m_settings.blackImagePath
                                        : m_settings.whiteImagePath;
            if (skin == AppCfg::PieceSkin::Custom && !imagePath.isEmpty()) {
                QImage image = loadImageCached(imagePath);
                if (!image.isNull()) {
                    QPainterPath path;
                    path.addEllipse(circleRect);
                    painter.save();
                    painter.setClipPath(path);
                    QImage scaled = image.scaled(circleRect.size().toSize(),
                                                 Qt::KeepAspectRatioByExpanding,
                                                 Qt::SmoothTransformation);
                    painter.drawImage(circleRect.topLeft(), scaled);
                    painter.restore();
                    continue;
                }
            }

            // 渐变棋子
            QColor inner = isBlack ? AppCfg::pieceBlackInner(skin)
                                   : AppCfg::pieceWhiteInner(skin);
            QColor outer = isBlack ? AppCfg::pieceBlackOuter(skin)
                                   : AppCfg::pieceWhiteOuter(skin);

            QRadialGradient gradient;
            gradient.setCenter(x, y);
            gradient.setFocalPoint(x - pieceRadius * 0.35,
                                   y - pieceRadius * 0.4);
            gradient.setRadius(pieceRadius * 1.15);
            gradient.setColorAt(0.0, inner);
            gradient.setColorAt(0.72, inner);
            gradient.setColorAt(1.0, outer);
            painter.setBrush(gradient);
            painter.drawEllipse(circleRect);

            // 高光
            QColor gloss(255, 255, 255,
                         isBlack ? 70 : 90);
            painter.setBrush(gloss);
            painter.drawEllipse(QRectF(x - pieceRadius * 0.55,
                                       y - pieceRadius * 0.6,
                                       pieceRadius * 0.5,
                                       pieceRadius * 0.35));
        }
    }
}

void BoardWidget::drawLastMoveMarker(QPainter& painter) {
    const auto& board = game->getBoard();
    auto lastMove = board.getLastMove();

    if (lastMove.has_value()) {
        auto [x, y] = gridToPos(lastMove->row, lastMove->col);
        painter.setPen(QPen(QColor(255, 0, 0), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(x - 4, y - 4, 8, 8);
    }
}

void BoardWidget::drawWinHighlight(QPainter& painter) {
    if (m_winCells.isEmpty()) {
        return;
    }

    double pulse = 1.0;
    if (m_settings.winEffect == AppCfg::WinEffect::Pulse) {
        double t = static_cast<double>(m_clock.elapsed());
        pulse = 0.75 + 0.25 * std::sin(t * 0.02);
    }

    QColor gold(255, 214, 90, static_cast<int>(190 * pulse));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(gold, 3));

    for (const auto& p : m_winCells) {
        auto [x, y] = gridToPos(p.row, p.col);
        int r = pieceRadius + 4;
        painter.drawEllipse(x - r, y - r, r * 2, r * 2);
    }
}

void BoardWidget::drawEffects(QPainter& painter) {
    qint64 now = m_clock.elapsed();

    for (const Effect& e : m_effects) {
        double progress = (now - e.startMs) / e.durationMs;
        if (progress < 0.0) {
            progress = 0.0;
        } else if (progress > 1.0) {
            progress = 1.0;
        }

        if (e.kind == EffectKind::Ring) {
            double radius = pieceRadius * 0.6
                            + (pieceRadius * 2.6 - pieceRadius * 0.6) * progress;
            int alpha = static_cast<int>(220 * (1.0 - progress));
            QColor c = e.color;
            c.setAlpha(alpha);
            painter.setPen(QPen(c, 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(e.pos), radius, radius);
        } else { // Particle
            QPointF current(e.pos.x() + e.dx * progress,
                            e.pos.y() + e.dy * progress);
            int alpha = static_cast<int>(255 * (1.0 - progress));
            QColor c = e.color;
            c.setAlpha(alpha);
            painter.setPen(Qt::NoPen);
            painter.setBrush(c);
            painter.drawEllipse(current, e.size * 0.5, e.size * 0.5);
        }
    }
}

void BoardWidget::triggerPlaceEffect(int row, int col, int player) {
    if (m_settings.placeEffect == AppCfg::PlaceEffect::None) {
        return;
    }

    auto [x, y] = gridToPos(row, col);
    qint64 now = m_clock.elapsed();

    if (m_settings.placeEffect == AppCfg::PlaceEffect::Ring) {
        Effect e;
        e.kind = EffectKind::Ring;
        e.pos = QPointF(x, y);
        e.startMs = now;
        e.durationMs = 550.0;
        e.color = (player == 0) ? QColor(255, 255, 255)
                                : QColor(255, 220, 120);
        m_effects.push_back(e);
    } else if (m_settings.placeEffect == AppCfg::PlaceEffect::Spark) {
        const std::vector<QColor> colors = {
            QColor("#ffffff"), QColor("#ffd76a"),
            QColor("#9fe8ff"), QColor("#ff9d9d")
        };
        spawnParticles(x, y, 10, 1.1, colors, now);
    }

    ensureEffectTimer();
}

void BoardWidget::triggerWinEffect(const std::vector<Gomoku::Position>& cells) {
    m_winCells.clear();
    for (const auto& p : cells) {
        m_winCells.push_back(p);
    }

    qint64 now = m_clock.elapsed();
    if (m_settings.winEffect == AppCfg::WinEffect::Burst && !cells.empty()) {
        const auto& mid = cells[cells.size() / 2];
        auto [x, y] = gridToPos(mid.row, mid.col);
        const std::vector<QColor> colors = {
            QColor("#ffd76a"), QColor("#ff9d9d"), QColor("#9fe8ff"),
            QColor("#b7f0a5"), QColor("#fff7d6")
        };
        spawnParticles(x, y, 24, 2.4, colors, now);
    }

    if (m_settings.winEffect == AppCfg::WinEffect::Pulse ||
        m_settings.winEffect == AppCfg::WinEffect::Burst) {
        m_winShake = true;
        m_shakeStartMs = now;
        m_shakeDurationMs = 500.0;
    }

    ensureEffectTimer();
}

void BoardWidget::clearBoardEffects() {
    m_effects.clear();
    m_winCells.clear();
    m_winShake = false;
    ensureEffectTimer();
    update();
}

void BoardWidget::spawnParticles(double x, double y, int count, double spread,
                                 const std::vector<QColor>& colors,
                                 qint64 startMs) {
    if (colors.empty()) {
        return;
    }

    auto* rng = QRandomGenerator::global();
    double cell = static_cast<double>(cellSize);
    double baseDist = cell * 0.7;

    for (int i = 0; i < count; ++i) {
        double angle = 2.0 * kPi * i / count
                       + (rng->generateDouble() - 0.5) * 0.7;
        double dist = baseDist * (0.7 + rng->generateDouble() * 0.9) * spread;

        Effect e;
        e.kind = EffectKind::Particle;
        e.pos = QPointF(x, y);
        e.startMs = startMs;
        e.durationMs = 750.0;
        e.dx = std::cos(angle) * dist;
        e.dy = std::sin(angle) * dist;
        e.size = std::max(4.0, cell * 0.18);
        e.color = colors[i % colors.size()];
        m_effects.push_back(e);
    }
}

QImage BoardWidget::loadImageCached(const QString& path) {
    auto it = m_imageCache.find(path);
    if (it != m_imageCache.end()) {
        return it.value();
    }
    QImage image(path);
    if (!image.isNull()) {
        m_imageCache.insert(path, image);
    }
    return image;
}

void BoardWidget::ensureEffectTimer() {
    bool need = !m_effects.isEmpty() || m_winShake ||
                (!m_winCells.isEmpty() &&
                 m_settings.winEffect == AppCfg::WinEffect::Pulse);
    if (need && !m_effectTimer.isActive()) {
        m_effectTimer.start();
    } else if (!need && m_effectTimer.isActive()) {
        m_effectTimer.stop();
    }
}

void BoardWidget::onEffectTick() {
    qint64 now = m_clock.elapsed();

    for (int i = m_effects.size() - 1; i >= 0; --i) {
        if (now - m_effects[i].startMs >= m_effects[i].durationMs) {
            m_effects.removeAt(i);
        }
    }

    if (m_winShake && now - m_shakeStartMs >= m_shakeDurationMs) {
        m_winShake = false;
    }

    ensureEffectTimer();
    update();
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

    int minDimension = qMin(width(), height());
    cellSize = (minDimension - 2 * margin) / (Gomoku::Board::SIZE - 1);
    pieceRadius = cellSize / 3;
}
