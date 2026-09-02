#include "MainWindow.h"
#include "BoardWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMenuBar>
#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , boardWidget(nullptr)
    , statusLabel(nullptr)
    , currentPlayerLabel(nullptr)
    , newGameButton(nullptr)
    , undoButton(nullptr)
    , m_isAIThinking(false)
    , m_aiWatcher(nullptr)
{
    setupUI();
    createMenus();

    game.onStateChanged([this](Gomoku::GameState state) {
        onGameStateChanged(state);
    });

    game.onMoveMade([this](int, int) {
        boardWidget->updateBoard();
        updateStatusBar();

        if (!m_isAIThinking &&
            game.isAITurn() &&
            game.getState() == Gomoku::GameState::InProgress) {
            scheduleAIMove();
        }
    });

    updateStatusBar();
}

MainWindow::~MainWindow() {
    aiPlayer.cancel();
    if (m_aiWatcher) {
        m_aiWatcher->waitForFinished();
    }
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    boardWidget = new BoardWidget(this);
    boardWidget->setGame(&game);
    connect(boardWidget, &BoardWidget::positionClicked,
            this, &MainWindow::onPositionClicked);
    mainLayout->addWidget(boardWidget);

    QWidget* controlPanel = new QWidget(this);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setSpacing(15);

    currentPlayerLabel = new QLabel("黑方回合", this);
    currentPlayerLabel->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    controlLayout->addWidget(currentPlayerLabel);

    statusLabel = new QLabel("准备开始", this);
    statusLabel->setStyleSheet("font-size: 14px; padding: 5px;");
    controlLayout->addWidget(statusLabel);

    controlLayout->addStretch();

    newGameButton = new QPushButton("新游戏", this);
    newGameButton->setMinimumHeight(40);
    newGameButton->setStyleSheet("font-size: 14px; padding: 8px;");
    connect(newGameButton, &QPushButton::clicked, this, &MainWindow::onNewGame);
    controlLayout->addWidget(newGameButton);

    undoButton = new QPushButton("悔棋", this);
    undoButton->setMinimumHeight(40);
    undoButton->setStyleSheet("font-size: 14px; padding: 8px;");
    connect(undoButton, &QPushButton::clicked, this, &MainWindow::onUndo);
    controlLayout->addWidget(undoButton);

    controlLayout->addStretch();

    mainLayout->addWidget(controlPanel);

    setWindowTitle("五子棋 - Gomoku");
    setMinimumSize(700, 600);
}

void MainWindow::createMenus() {
    QMenu* fileMenu = menuBar()->addMenu("文件(&F)");

    QAction* newGameAction = fileMenu->addAction("新游戏(&N)");
    connect(newGameAction, &QAction::triggered, this, &MainWindow::onNewGame);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("退出(&X)");
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu* gameMenu = menuBar()->addMenu("游戏(&G)");

    QAction* undoAction = gameMenu->addAction("悔棋(&U)");
    connect(undoAction, &QAction::triggered, this, &MainWindow::onUndo);

    QMenu* helpMenu = menuBar()->addMenu("帮助(&H)");

    QAction* aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于五子棋",
            "五子棋 (Gomoku)\n\n"
            "版本：1.0\n"
            "基于 C++ Qt 开发\n\n"
            "游戏规则：\n"
            "1. 黑方先行，双方轮流落子\n"
            "2. 先形成五子连珠者获胜\n"
            "3. 支持人机对战和双人对战\n\n"
            "《软件设计》课程项目");
    });
}

void MainWindow::onNewGame() {
    Gomoku::GameModeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        startNewGameWithConfig(dialog.getConfig());
    }
}

void MainWindow::startNewGameWithConfig(const Gomoku::GameConfig& config) {
    aiPlayer.cancel();
    if (m_aiWatcher) {
        m_aiWatcher->waitForFinished();
    }

    game.startNewGame();
    game.setPlayerType(Gomoku::ChessPiece::Black, Gomoku::PlayerType::Human);
    game.setPlayerType(Gomoku::ChessPiece::White, Gomoku::PlayerType::Human);

    if (config.isHumanVsAI) {
        if (config.humanColor == Gomoku::PlayerColor::Black) {
            game.setPlayerType(Gomoku::ChessPiece::Black, Gomoku::PlayerType::Human);
            game.setPlayerType(Gomoku::ChessPiece::White, Gomoku::PlayerType::AI);
        } else {
            game.setPlayerType(Gomoku::ChessPiece::Black, Gomoku::PlayerType::AI);
            game.setPlayerType(Gomoku::ChessPiece::White, Gomoku::PlayerType::Human);
        }

        Gomoku::AIConfig aiConfig;
        switch (config.aiDifficulty) {
            case Gomoku::AIDifficultyLevel::Easy:
                aiConfig.maxDepth = 3;
                aiConfig.timeLimitMs = 500;
                break;
            case Gomoku::AIDifficultyLevel::Normal:
                aiConfig.maxDepth = 4;
                aiConfig.timeLimitMs = 2000;
                break;
            case Gomoku::AIDifficultyLevel::Hard:
                aiConfig.maxDepth = 6;
                aiConfig.timeLimitMs = 5000;
                break;
        }
        aiPlayer.setConfig(aiConfig);
    }

    boardWidget->updateBoard();
    updateStatusBar();

    if (config.isHumanVsAI &&
        config.humanColor == Gomoku::PlayerColor::White) {
        QTimer::singleShot(100, this, [this]() {
            scheduleAIMove();
        });
    }
}

void MainWindow::scheduleAIMove() {
    if (m_isAIThinking) {
        return;
    }
    if (!game.isAITurn() ||
        game.getState() != Gomoku::GameState::InProgress) {
        return;
    }

    setAIThinkingState(true);

    auto snapshot = game.getGameStateSnapshot();
    auto* watcher = new QFutureWatcher<Gomoku::SearchResult>(this);
    m_aiWatcher = watcher;

    connect(watcher, &QFutureWatcher<Gomoku::SearchResult>::finished,
            this, [this, watcher]() {
        auto result = watcher->result();
        if (result.valid &&
            game.getState() == Gomoku::GameState::InProgress &&
            game.isAITurn()) {
            game.makeMove(result.move.row, result.move.col);
            boardWidget->updateBoard();
            updateStatusBar();
        }
        setAIThinkingState(false);
        if (m_aiWatcher == watcher) {
            m_aiWatcher = nullptr;
        }
        watcher->deleteLater();
    });

    auto future = QtConcurrent::run([this, snapshot]() {
        return aiPlayer.search(snapshot);
    });
    watcher->setFuture(future);
}

void MainWindow::onUndo() {
    if (m_isAIThinking) {
        QMessageBox::information(this, "提示", "AI 思考中，请稍候...");
        return;
    }

    if (game.isHumanVsAI()) {
        game.undoMove();
        game.undoMove();
    } else {
        game.undoMove();
    }

    boardWidget->updateBoard();
    updateStatusBar();
}

void MainWindow::onPositionClicked(int row, int col) {
    if (m_isAIThinking) {
        return;
    }
    if (game.isAITurn()) {
        return;
    }

    auto result = game.makeMove(row, col);

    switch (result) {
        case Gomoku::MoveResult::Success:
            break;

        case Gomoku::MoveResult::InvalidPosition:
            QMessageBox::warning(this, "提示", "无效的位置");
            break;

        case Gomoku::MoveResult::PositionOccupied:
            QMessageBox::warning(this, "提示", "该位置已有棋子");
            break;

        case Gomoku::MoveResult::GameEnded:
            QMessageBox::information(this, "提示", "游戏已结束，请开始新游戏");
            break;
    }
}

void MainWindow::onGameStateChanged(Gomoku::GameState state) {
    updateStatusBar();

    switch (state) {
        case Gomoku::GameState::BlackWin:
            QMessageBox::information(this, "游戏结束", "黑方获胜！");
            break;

        case Gomoku::GameState::WhiteWin:
            QMessageBox::information(this, "游戏结束", "白方获胜！");
            break;

        case Gomoku::GameState::Draw:
            QMessageBox::information(this, "游戏结束", "平局！");
            break;

        default:
            break;
    }
}

void MainWindow::setAIThinkingState(bool thinking) {
    m_isAIThinking = thinking;
    boardWidget->setEnabled(!thinking);

    if (thinking) {
        statusLabel->setText("AI 思考中...");
        currentPlayerLabel->setText("AI 计算中");
        QApplication::setOverrideCursor(Qt::WaitCursor);
    } else {
        updateStatusBar();
        QApplication::restoreOverrideCursor();
    }
}

void MainWindow::updateStatusBar() {
    auto state = game.getState();

    switch (state) {
        case Gomoku::GameState::NotStarted:
            statusLabel->setText("点击\"新游戏\"开始");
            currentPlayerLabel->setText("准备开始");
            break;

        case Gomoku::GameState::InProgress:
            statusLabel->setText("游戏进行中");
            if (game.getCurrentPlayer() == Gomoku::ChessPiece::Black) {
                currentPlayerLabel->setText("黑方回合");
            } else {
                currentPlayerLabel->setText("白方回合");
            }
            break;

        case Gomoku::GameState::BlackWin:
            statusLabel->setText("游戏结束");
            currentPlayerLabel->setText("黑方获胜!");
            break;

        case Gomoku::GameState::WhiteWin:
            statusLabel->setText("游戏结束");
            currentPlayerLabel->setText("白方获胜!");
            break;

        case Gomoku::GameState::Draw:
            statusLabel->setText("游戏结束");
            currentPlayerLabel->setText("平局");
            break;
    }
}