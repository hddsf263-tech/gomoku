#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QFutureWatcher>
#include <QTimer>
#include <QSettings>

#include "../include/Game.h"
#include "../include/NetworkManager.h"
#include "GameModeDialog.h"
#include "ai/AIPlayer.h"
#include "ai/SearchResult.h"

class BoardWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewGame();
    void onUndo();
    void onPositionClicked(int row, int col);
    void onGameStateChanged(Gomoku::GameState state);
    void onSurrenderClicked();
    void onDrawClicked();
    void onReplayClicked();
    void onReplayStepPrev();
    void onReplayStepNext();
    void onReplayExit();
    void onShowStats();
    void onTimerTick();
    void handleTimeout();

private:
    void setupUI();
    void createMenus();
    void updateStatusBar();
    void updateButtons();
    void updateMoveCount();
    void updateTimerLabel();
    void updateReplayStepLabel();
    void startNewGameWithConfig(const Gomoku::GameConfig& config);
    void startNetworkGame(const Gomoku::GameConfig& config);
    void scheduleAIMove();
    void setAIThinkingState(bool thinking);
    void resetMoveTimer();
    void startMoveTimer();
    void stopMoveTimer();
    void recordGameResult(Gomoku::GameState state);
    void showStatsDialog();
    void enterReplayMode();
    void leaveReplayMode();

    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkError(const QString& message);
    void onNetworkMove(int row, int col);
    void onNetworkHello(int color);
    void onNetworkReset();
    void onNetworkSurrender();
    void onNetworkDrawOffer();
    void onNetworkDrawResponse(bool accept);

    Gomoku::Game game;
    Gomoku::AIPlayer aiPlayer;
    Gomoku::NetworkManager network;
    BoardWidget* boardWidget;
    QLabel* statusLabel;
    QLabel* currentPlayerLabel;
    QLabel* moveCountLabel;
    QLabel* timerLabel;
    QPushButton* newGameButton;
    QPushButton* undoButton;
    QPushButton* surrenderButton;
    QPushButton* drawButton;
    QPushButton* replayButton;
    QWidget* replayControls;
    QPushButton* replayPrevButton;
    QPushButton* replayNextButton;
    QPushButton* replayExitButton;
    bool m_isAIThinking;
    QFutureWatcher<Gomoku::SearchResult>* m_aiWatcher;
    QTimer* m_moveTimer;
    int m_moveTimeSeconds;
    int m_currentTimeLeft;
    bool m_drawPending;
    bool m_enableTimer;

    Gomoku::GameConfig m_config;
    Gomoku::ChessPiece m_myColor;
};

#endif // MAINWINDOW_H
