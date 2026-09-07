#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QElapsedTimer>
#include <QTimer>
#include <QImage>
#include <QHash>
#include <QVector>
#include <vector>
#include "../include/Board.h"
#include "../include/Game.h"
#include "AppSettings.h"

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

    /// @brief 应用外观设置（皮肤/特效）
    void configure(const AppCfg::AppSettings& settings);

    /// @brief 触发落子特效
    void triggerPlaceEffect(int row, int col, int player);

    /// @brief 触发获胜特效
    void triggerWinEffect(const std::vector<Gomoku::Position>& cells);

    /// @brief 清除所有特效（新游戏时调用）
    void clearBoardEffects();

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

    /// @brief 特效类型
    enum class EffectKind { Ring, Particle };
    struct Effect {
        EffectKind kind;
        QPointF pos;
        qint64 startMs;
        double durationMs;
        QColor color;
        double dx = 0.0;
        double dy = 0.0;
        double size = 4.0;
    };

    void drawBoardBackground(QPainter& painter);
    void drawWinHighlight(QPainter& painter);
    void drawEffects(QPainter& painter);
    void spawnParticles(double x, double y, int count, double spread,
                        const std::vector<QColor>& colors, qint64 startMs);
    QImage loadImageCached(const QString& path);
    void ensureEffectTimer();
    void onEffectTick();

    Gomoku::Game* game;                      ///< 游戏对象指针
    int cellSize;                            ///< 格子大小
    int margin;                              ///< 边距
    int pieceRadius;                         ///< 棋子半径

    AppCfg::AppSettings m_settings;          ///< 皮肤与特效设置
    QVector<Effect> m_effects;               ///< 活动特效
    QVector<Gomoku::Position> m_winCells;    ///< 获胜连线
    QElapsedTimer m_clock;                   ///< 特效计时器
    QTimer m_effectTimer;                    ///< 特效刷新定时器
    bool m_winShake = false;                 ///< 是否触发棋盘抖动
    qint64 m_shakeStartMs = 0;               ///< 抖动开始时间
    double m_shakeDurationMs = 500.0;        ///< 抖动时长
    QHash<QString, QImage> m_imageCache;     ///< 图片缓存
};

#endif // BOARDWIDGET_H
