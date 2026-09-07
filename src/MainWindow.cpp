#include "MainWindow.h"
#include "BoardWidget.h"
#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , boardWidget(nullptr)
    , statusLabel(nullptr)
    , currentPlayerLabel(nullptr)
    , moveInfoLabel(nullptr)
    , newGameButton(nullptr)
    , undoButton(nullptr)
    , m_isAIThinking(false)
    , m_aiWatcher(nullptr)
    , m_myColor(Gomoku::ChessPiece::Black)
    , m_soundMenuAction(nullptr)
    , m_soundToolAction(nullptr)
{
    setupUI();
    createMenus();

    m_settings = AppCfg::AppSettings::load();
    refreshSettings();

    game.onStateChanged([this](Gomoku::GameState state) {
        onGameStateChanged(state);
    });

    game.onMoveMade([this](int row, int col) {
        int player = (game.getCurrentPlayer() == Gomoku::ChessPiece::Black) ? 0 : 1;
        m_soundPlayer.playPlaceSound(player);
        boardWidget->triggerPlaceEffect(row, col, player);
        boardWidget->updateBoard();
        updateStatusBar();

        if (!m_isAIThinking &&
            game.isAITurn() &&
            game.getState() == Gomoku::GameState::InProgress) {
            scheduleAIMove();
        }
    });

    network.onConnected([this]() {
        onNetworkConnected();
    });
    network.onDisconnected([this]() {
        onNetworkDisconnected();
    });
    network.onError([this](const QString& message) {
        onNetworkError(message);
    });
    network.onMove([this](int row, int col) {
        onNetworkMove(row, col);
    });
    network.onHello([this](int color) {
        onNetworkHello(color);
    });
    network.onReset([this]() {
        onNetworkReset();
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

    moveInfoLabel = new QLabel("等待落子", this);
    moveInfoLabel->setStyleSheet("font-size: 13px; padding: 5px; color: #555;");
    controlLayout->addWidget(moveInfoLabel);

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

    QToolBar* toolBar = addToolBar("音效");
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_soundToolAction = toolBar->addAction("音效：开");
    m_soundToolAction->setCheckable(true);
    m_soundToolAction->setChecked(true);
    connect(m_soundToolAction, &QAction::toggled,
            this, &MainWindow::onToggleSound);
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

    QMenu* settingsMenu = menuBar()->addMenu("设置(&S)");

    QAction* skinAction = settingsMenu->addAction("外观与音效(&P)");
    connect(skinAction, &QAction::triggered, this, &MainWindow::onOpenSettings);

    settingsMenu->addSeparator();

    m_soundMenuAction = settingsMenu->addAction("音效开关(&M)");
    m_soundMenuAction->setCheckable(true);
    m_soundMenuAction->setChecked(true);
    connect(m_soundMenuAction, &QAction::toggled,
            this, &MainWindow::onToggleSound);

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
            "3. 支持人机对战、双人对战和网络对战\n\n"
            "《软件设计》课程项目");
    });
}

void MainWindow::onNewGame() {
    if (m_config.isNetwork) {
        if (network.isConnected()) {
            network.sendReset();
            game.startNewGame();
            game.setPlayerType(Gomoku::ChessPiece::Black, Gomoku::PlayerType::Human);
            game.setPlayerType(Gomoku::ChessPiece::White, Gomoku::PlayerType::Human);
            boardWidget->clearBoardEffects();
            boardWidget->updateBoard();
            updateStatusBar();
            return;
        }
        // 已断开：关闭网络实例，让用户回到模式选择
        network.disconnectPeer();
    }

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

    m_config = config;

    if (config.isNetwork) {
        startNetworkGame(config);
        return;
    }

    boardWidget->clearBoardEffects();
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

void MainWindow::startNetworkGame(const Gomoku::GameConfig& config) {
    game.startNewGame();
    game.setPlayerType(Gomoku::ChessPiece::Black, Gomoku::PlayerType::Human);
    game.setPlayerType(Gomoku::ChessPiece::White, Gomoku::PlayerType::Human);

    boardWidget->setEnabled(false);
    undoButton->setEnabled(false);
    boardWidget->updateBoard();

    if (config.networkIsHost) {
        m_myColor = Gomoku::ChessPiece::Black;
        bool ok = network.listen(config.networkPort);
        if (!ok) {
            m_config = Gomoku::GameConfig();
            undoButton->setEnabled(true);
            boardWidget->setEnabled(true);
            QMessageBox::warning(this, "错误", "端口监听失败，请更换端口后重试");
            statusLabel->setText("端口监听失败");
            setWindowTitle("五子棋 - Gomoku");
            return;
        }
        setWindowTitle("五子棋 - 网络对战（主机）");
        statusLabel->setText("已开启房间，等待对手加入...");
        currentPlayerLabel->setText("主机 · 黑方");
    } else {
        m_myColor = Gomoku::ChessPiece::White;
        network.connectToHost(config.networkHost, config.networkPort);
        setWindowTitle("五子棋 - 网络对战（客户端）");
        statusLabel->setText("正在连接主机...");
        currentPlayerLabel->setText("客户端 · 白方");
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
    if (m_config.isNetwork) {
        QMessageBox::information(this, "提示", "网络对战暂不支持悔棋");
        return;
    }

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
    if (m_config.isNetwork) {
        if (!network.isConnected()) {
            statusLabel->setText("尚未连接对手");
            return;
        }
        if (game.isAITurn() ||
            game.getCurrentPlayer() != m_myColor) {
            statusLabel->setText("请等待对手落子");
            return;
        }

        auto result = game.makeMove(row, col);
        if (result == Gomoku::MoveResult::Success) {
            network.sendMove(row, col);
        } else {
            switch (result) {
                case Gomoku::MoveResult::InvalidPosition:
                    QMessageBox::warning(this, "提示", "无效的位置");
                    break;
                case Gomoku::MoveResult::PositionOccupied:
                    QMessageBox::warning(this, "提示", "该位置已有棋子");
                    break;
                case Gomoku::MoveResult::GameEnded:
                    QMessageBox::information(this, "提示", "游戏已结束，请开始新游戏");
                    break;
                default:
                    break;
            }
        }
        return;
    }

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

    if (state == Gomoku::GameState::BlackWin ||
        state == Gomoku::GameState::WhiteWin) {
        Gomoku::ChessPiece winner = game.getCurrentPlayer();
        auto lastMove = game.getBoard().getLastMove();
        if (lastMove.has_value()) {
            auto winCells = game.getBoard().getWinningLine(
                lastMove->row, lastMove->col, winner);
            boardWidget->triggerWinEffect(winCells);
            m_soundPlayer.playWinSound();
        }
    }

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

void MainWindow::onNetworkConnected() {
    boardWidget->setEnabled(true);
    undoButton->setEnabled(false);

    if (network.role() == Gomoku::NetworkManager::Role::Host) {
        m_myColor = Gomoku::ChessPiece::Black;
        network.sendHello(0);
        statusLabel->setText("对手已加入，黑方先行");
        currentPlayerLabel->setText("主机 · 黑方");
    } else {
        m_myColor = Gomoku::ChessPiece::White;
        network.sendHello(1);
        statusLabel->setText("已连接到主机，等待黑方落子");
        currentPlayerLabel->setText("客户端 · 白方");
    }

    updateStatusBar();
}

void MainWindow::onNetworkDisconnected() {
    boardWidget->setEnabled(false);
    undoButton->setEnabled(false);
    statusLabel->setText("对手已断开连接");
    currentPlayerLabel->setText("连接断开");
    QMessageBox::information(this, "提示", "对手已断开连接，请重新开始游戏。");
}

void MainWindow::onNetworkError(const QString& message) {
    statusLabel->setText("网络错误：" + message);
    QMessageBox::warning(this, "网络错误", message);
}

void MainWindow::onNetworkMove(int row, int col) {
    if (!network.isConnected()) {
        return;
    }
    if (game.getState() != Gomoku::GameState::InProgress) {
        return;
    }
    // 收到的移动应来自对手：当前轮到对手（即不是自己）。
    if (game.getCurrentPlayer() == m_myColor) {
        return;
    }
    game.makeMove(row, col);
}

void MainWindow::onNetworkHello(int color) {
    // color 是对方颜色，自己取相反颜色，保证两端一致。
    m_myColor = (color == 0) ? Gomoku::ChessPiece::White
                             : Gomoku::ChessPiece::Black;
    updateStatusBar();
}

void MainWindow::onNetworkReset() {
    game.startNewGame();
    game.setPlayerType(Gomoku::ChessPiece::Black, Gomoku::PlayerType::Human);
    game.setPlayerType(Gomoku::ChessPiece::White, Gomoku::PlayerType::Human);
    boardWidget->clearBoardEffects();
    boardWidget->updateBoard();
    updateStatusBar();
}

void MainWindow::onOpenSettings() {
    SettingsDialog dialog(m_settings, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_settings = dialog.getSettings();
        m_settings.save();
        refreshSettings();
    }
}

void MainWindow::onToggleSound(bool enabled) {
    m_settings.soundEnabled = enabled;
    m_settings.save();
    refreshSettings();
}

void MainWindow::refreshSettings() {
    m_soundPlayer.configure(m_settings);
    if (boardWidget) {
        boardWidget->configure(m_settings);
    }

    if (m_soundMenuAction) {
        m_soundMenuAction->blockSignals(true);
        m_soundMenuAction->setChecked(m_settings.soundEnabled);
        m_soundMenuAction->blockSignals(false);
    }
    if (m_soundToolAction) {
        m_soundToolAction->blockSignals(true);
        m_soundToolAction->setChecked(m_settings.soundEnabled);
        m_soundToolAction->setText(m_settings.soundEnabled ? "音效：开" : "音效：关");
        m_soundToolAction->blockSignals(false);
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
            if (m_config.isNetwork && network.isConnected()) {
                if (game.getCurrentPlayer() == m_myColor) {
                    currentPlayerLabel->setText("你的回合");
                } else {
                    currentPlayerLabel->setText("等待对方落子");
                }
            } else {
                if (game.getCurrentPlayer() == Gomoku::ChessPiece::Black) {
                    currentPlayerLabel->setText("黑方回合");
                } else {
                    currentPlayerLabel->setText("白方回合");
                }
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

    // 状态栏显示当前手数与最后一步坐标（与网页版一致，如 "12 手 · C8"）
    const auto& moves = game.getBoard().getMoveHistory();
    if (moves.empty()) {
        moveInfoLabel->setText("等待落子");
    } else {
        const auto& last = moves.back();
        QString coord = QString(QChar('A' + last.col)) + QString::number(last.row + 1);
        moveInfoLabel->setText(QString("%1 手 · %2").arg(moves.size()).arg(coord));
    }
}
