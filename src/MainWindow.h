#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QFutureWatcher>

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

private:
    void setupUI();
    void createMenus();
    void updateStatusBar();
    void startNewGameWithConfig(const Gomoku::GameConfig& config);
    void startNetworkGame(const Gomoku::GameConfig& config);
    void scheduleAIMove();
    void setAIThinkingState(bool thinking);

    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkError(const QString& message);
    void onNetworkMove(int row, int col);
    void onNetworkHello(int color);
    void onNetworkReset();

    Gomoku::Game game;
    Gomoku::AIPlayer aiPlayer;
    Gomoku::NetworkManager network;
    BoardWidget* boardWidget;
    QLabel* statusLabel;
    QLabel* currentPlayerLabel;
    QPushButton* newGameButton;
    QPushButton* undoButton;
    bool m_isAIThinking;
    QFutureWatcher<Gomoku::SearchResult>* m_aiWatcher;

    Gomoku::GameConfig m_config;
    Gomoku::ChessPiece m_myColor;
};

#endif // MAINWINDOW_H
