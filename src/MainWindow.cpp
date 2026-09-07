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
    , moveCountLabel(nullptr)
    , timerLabel(nullptr)
    , newGameButton(nullptr)
    , undoButton(nullptr)
    , surrenderButton(nullptr)
    , drawButton(nullptr)
    , replayButton(nullptr)
    , replayControls(nullptr)
    , replayPrevButton(nullptr)
    , replayNextButton(nullptr)
    , replayExitButton(nullptr)
    , m_isAIThinking(false)
    , m_aiWatcher(nullptr)
    , m_moveTimer(nullptr)
    , m_moveTimeSeconds(30)
    , m_currentTimeLeft(30)
    , m_drawPending(false)
    , m_enableTimer(true)
    , m_myColor(Gomoku::ChessPiece::Black)
{
    setupUI();
    createMenus();

    game.onStateChanged([this](Gomoku::GameState state) {
        onGameStateChanged(state);
    });

    game.onMoveMade([this](int, int) {
        boardWidget->updateBoard();
        updateStatusBar();
        updateMoveCount();
        updateButtons();
        resetMoveTimer();

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
    network.onSurrender([this]() {
        onNetworkSurrender();
    });
    network.onDrawOffer([this]() {
        onNetworkDrawOffer();
    });
    network.onDrawResponse([this](bool accept) {
        onNetworkDrawResponse(accept);
    });

    m_moveTimer = new QTimer(this);
    m_moveTimer->setInterval(1000);
    connect(m_moveTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    updateStatusBar();
    updateButtons();
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

    moveCountLabel = new QLabel("步数：0", this);
    moveCountLabel->setStyleSheet("font-size: 14px; padding: 5px;");
    controlLayout->addWidget(moveCountLabel);

    timerLabel = new QLabel("计时：未开启", this);
    timerLabel->setStyleSheet("font-size: 14px; padding: 5px;");
    controlLayout->addWidget(timerLabel);

    controlLayout->addStretch();

    surrenderButton = new QPushButton("投降", this);
    surrenderButton->setMinimumHeight(36);
    surrenderButton->setStyleSheet("font-size: 14px; padding: 8px;");
    connect(surrenderButton, &QPushButton::clicked, this, &MainWindow::onSurrenderClicked);
    controlLayout->addWidget(surrenderButton);

    drawButton = new QPushButton("求和棋", this);
    drawButton->setMinimumHeight(36);
    drawButton->setStyleSheet("font-size: 14px; padding: 8px;");
    connect(drawButton, &QPushButton::clicked, this, &MainWindow::onDrawClicked);
    controlLayout->addWidget(drawButton);

    replayButton = new QPushButton("回放对局", this);
    replayButton->setMinimumHeight(36);
    replayButton->setStyleSheet("font-size: 14px; padding: 8px;");
    connect(replayButton, &QPushButton::clicked, this, &MainWindow::onReplayClicked);
    controlLayout->addWidget(replayButton);

    replayControls = new QWidget(this);
    auto* replayLayout = new QHBoxLayout(replayControls);
    replayLayout->setContentsMargins(0, 0, 0, 0);
    replayLayout->setSpacing(6);

    replayPrevButton = new QPushButton("◀", this);
    replayPrevButton->setFixedHeight(36);
    connect(replayPrevButton, &QPushButton::clicked, this, &MainWindow::onReplayStepPrev);
    replayLayout->addWidget(replayPrevButton);

    replayNextButton = new QPushButton("▶", this);
    replayNextButton->setFixedHeight(36);
    connect(replayNextButton, &QPushButton::clicked, this, &MainWindow::onReplayStepNext);
    replayLayout->addWidget(replayNextButton);

    replayExitButton = new QPushButton("退出回放", this);
    replayExitButton->setFixedHeight(36);
    connect(replayExitButton, &QPushButton::clicked, this, &MainWindow::onReplayExit);
    replayLayout->addWidget(replayExitButton);

    replayControls->hide();
    controlLayout->addWidget(replayControls);

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

    gameMenu->addSeparator();

    QAction* statsAction = gameMenu->addAction("战绩统计(&S)");
    connect(statsAction, &QAction::triggered, this, &MainWindow::onShowStats);

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
            m_drawPending = false;
            m_currentTimeLeft = m_moveTimeSeconds;
            boardWidget->updateBoard();
            boardWidget->setEnabled(true);
            updateStatusBar();
            updateMoveCount();
            updateButtons();
            updateTimerLabel();
            startMoveTimer();
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
    m_enableTimer = config.enableTimer;
    m_moveTimeSeconds = config.moveTimeSeconds;
    m_currentTimeLeft = m_moveTimeSeconds;
    m_drawPending = false;

    if (config.isNetwork) {
        startNetworkGame(config);
        return;
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
    updateMoveCount();
    updateButtons();
    startMoveTimer();

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
    m_currentTimeLeft = m_moveTimeSeconds;
    updateMoveCount();
    updateButtons();
    stopMoveTimer();
    updateTimerLabel();

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
    updateMoveCount();

    bool ended = (state == Gomoku::GameState::BlackWin ||
                  state == Gomoku::GameState::WhiteWin ||
                  state == Gomoku::GameState::Draw);

    if (ended) {
        stopMoveTimer();
        recordGameResult(state);
        boardWidget->setEnabled(false);
    }

    updateButtons();

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
    undoButton->setEnabled(false);
    m_drawPending = false;

    if (network.role() == Gomoku::NetworkManager::Role::Host) {
        m_myColor = Gomoku::ChessPiece::Black;
        network.sendHello(0);
        statusLabel->setText("对手已加入，等待确认...");
        currentPlayerLabel->setText("主机 · 黑方");
    } else {
        m_myColor = Gomoku::ChessPiece::White;
        network.sendHello(1);
        statusLabel->setText("已连接到主机，等待确认...");
        currentPlayerLabel->setText("客户端 · 白方");
    }

    // 棋盘启用推迟到收到对手 HELLO 确认之后，避免回合/颜色竞态。
    boardWidget->setEnabled(false);
    updateStatusBar();
    updateButtons();
    updateTimerLabel();
}

void MainWindow::onNetworkDisconnected() {
    boardWidget->setEnabled(false);
    undoButton->setEnabled(false);
    stopMoveTimer();
    if (m_config.isNetwork) {
        network.disconnectPeer();
    }
    statusLabel->setText("对手已断开连接");
    currentPlayerLabel->setText("连接断开");
    QMessageBox::information(this, "提示", "对手已断开连接，请重新开始游戏。");
    updateButtons();
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

    // 双方颜色确认完成后才启用棋盘，开始计时。
    m_drawPending = false;
    m_currentTimeLeft = m_moveTimeSeconds;
    boardWidget->setEnabled(true);
    if (m_config.isNetwork) {
        statusLabel->setText("对局开始，双方回合已确认");
    }
    updateStatusBar();
    updateButtons();
    updateTimerLabel();
    startMoveTimer();
}

void MainWindow::onNetworkReset() {
    game.startNewGame();
    game.setPlayerType(Gomoku::ChessPiece::Black, Gomoku::PlayerType::Human);
    game.setPlayerType(Gomoku::ChessPiece::White, Gomoku::PlayerType::Human);
    boardWidget->updateBoard();
    m_drawPending = false;
    m_currentTimeLeft = m_moveTimeSeconds;
    boardWidget->setEnabled(true);
    updateStatusBar();
    updateMoveCount();
    updateButtons();
    updateTimerLabel();
    startMoveTimer();
}

void MainWindow::onNetworkSurrender() {
    if (game.getState() != Gomoku::GameState::InProgress) {
        return;
    }

    // 对方认输：对方颜色作为败方，自己获胜。
    Gomoku::ChessPiece opponent =
        (m_myColor == Gomoku::ChessPiece::Black) ? Gomoku::ChessPiece::White
                                                  : Gomoku::ChessPiece::Black;
    game.forfeit(opponent);
    statusLabel->setText("对方认输，你获胜！");
    updateButtons();
}

void MainWindow::onNetworkDrawOffer() {
    if (game.getState() != Gomoku::GameState::InProgress) {
        return;
    }

    QMessageBox::StandardButton ret = QMessageBox::question(
        this, "求和棋", "对方提出和棋申请，是否同意？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        network.sendDrawResponse(true);
        game.declareDraw();
        statusLabel->setText("已同意和棋，本局平局");
    } else {
        network.sendDrawResponse(false);
        statusLabel->setText("已拒绝和棋，对局继续");
    }
    updateButtons();
}

void MainWindow::onNetworkDrawResponse(bool accept) {
    if (accept) {
        if (game.getState() == Gomoku::GameState::InProgress) {
            game.declareDraw();
        }
        statusLabel->setText("对方同意和棋，本局平局");
    } else {
        statusLabel->setText("对方拒绝了和棋，对局继续");
    }
    m_drawPending = false;
    updateButtons();
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
}

void MainWindow::updateMoveCount() {
    if (!moveCountLabel) {
        return;
    }
    if (boardWidget && boardWidget->isReplayMode()) {
        return;
    }
    moveCountLabel->setText(QString("步数：%1").arg(game.getMoveCount()));
}

void MainWindow::updateTimerLabel() {
    if (!timerLabel) {
        return;
    }

    if (!m_enableTimer) {
        timerLabel->setText("计时：未开启");
        return;
    }

    Gomoku::GameState state = game.getState();
    if (state == Gomoku::GameState::NotStarted) {
        timerLabel->setText("计时：等待开始");
        return;
    }
    if (state != Gomoku::GameState::InProgress) {
        timerLabel->setText("计时：对局结束");
        return;
    }

    Gomoku::ChessPiece cur = game.getCurrentPlayer();
    QString side = (cur == Gomoku::ChessPiece::Black) ? "黑方" : "白方";
    if (m_config.isNetwork && network.isConnected()) {
        side = (cur == m_myColor) ? "你的回合" : "对方回合";
    }
    timerLabel->setText(QString("%1 剩余 %2s").arg(side).arg(m_currentTimeLeft));
}

void MainWindow::updateReplayStepLabel() {
    if (!moveCountLabel) {
        return;
    }
    if (boardWidget && boardWidget->isReplayMode()) {
        int total = game.getMoveCount();
        moveCountLabel->setText(
            QString("回放：%1 / %2").arg(boardWidget->getReplayStep()).arg(total));
    } else {
        updateMoveCount();
    }
}

void MainWindow::updateButtons() {
    if (!surrenderButton) {
        return;
    }

    bool inProgress = (game.getState() == Gomoku::GameState::InProgress);
    bool ended =
        (game.getState() == Gomoku::GameState::BlackWin ||
         game.getState() == Gomoku::GameState::WhiteWin ||
         game.getState() == Gomoku::GameState::Draw);
    bool inReplay = (boardWidget && boardWidget->isReplayMode());

    surrenderButton->setEnabled(inProgress);
    drawButton->setEnabled(inProgress && !m_drawPending);
    replayButton->setEnabled(ended && game.getMoveCount() > 0 && !inReplay);
    newGameButton->setEnabled(true);

    if (m_config.isNetwork) {
        undoButton->setEnabled(false);
    } else {
        undoButton->setEnabled(inProgress && game.getMoveCount() > 0 && !inReplay);
    }
}

void MainWindow::resetMoveTimer() {
    m_currentTimeLeft = m_moveTimeSeconds;
    updateTimerLabel();
}

void MainWindow::startMoveTimer() {
    if (!m_enableTimer) {
        updateTimerLabel();
        return;
    }
    if (m_moveTimer && !m_moveTimer->isActive()) {
        m_moveTimer->start();
    }
}

void MainWindow::stopMoveTimer() {
    if (m_moveTimer && m_moveTimer->isActive()) {
        m_moveTimer->stop();
    }
    updateTimerLabel();
}

void MainWindow::onTimerTick() {
    if (!m_enableTimer) {
        return;
    }

    if (game.getState() != Gomoku::GameState::InProgress) {
        stopMoveTimer();
        return;
    }

    bool shouldTime = false;
    if (m_config.isNetwork) {
        shouldTime = network.isConnected() &&
                     (game.getCurrentPlayer() == m_myColor);
    } else {
        shouldTime =
            (game.getPlayerType(game.getCurrentPlayer()) ==
             Gomoku::PlayerType::Human);
    }

    if (!shouldTime) {
        m_currentTimeLeft = m_moveTimeSeconds;
        updateTimerLabel();
        return;
    }

    if (m_currentTimeLeft > 0) {
        --m_currentTimeLeft;
        updateTimerLabel();
    }
    if (m_currentTimeLeft <= 0) {
        handleTimeout();
    }
}

void MainWindow::handleTimeout() {
    if (game.getState() != Gomoku::GameState::InProgress) {
        return;
    }

    stopMoveTimer();
    Gomoku::ChessPiece loser = game.getCurrentPlayer();
    if (m_config.isNetwork) {
        loser = m_myColor;
    }
    game.forfeit(loser);
    statusLabel->setText("超时判负");
    if (m_config.isNetwork && network.isConnected()) {
        network.sendSurrender();
    }
    updateButtons();
}

void MainWindow::onSurrenderClicked() {
    if (game.getState() != Gomoku::GameState::InProgress) {
        return;
    }
    if (m_config.isNetwork && !network.isConnected()) {
        return;
    }

    QMessageBox::StandardButton ret = QMessageBox::question(
        this, "确认投降", "确定要认输吗？本局将由对方获胜。",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    Gomoku::ChessPiece loser =
        m_config.isNetwork ? m_myColor : game.getCurrentPlayer();
    game.forfeit(loser);
    statusLabel->setText("已投降");
    if (m_config.isNetwork && network.isConnected()) {
        network.sendSurrender();
    }
    updateButtons();
}

void MainWindow::onDrawClicked() {
    if (game.getState() != Gomoku::GameState::InProgress) {
        return;
    }

    if (m_config.isNetwork) {
        if (!network.isConnected()) {
            return;
        }
        if (m_drawPending) {
            statusLabel->setText("已向对方发出和棋申请，等待回应...");
            return;
        }
        m_drawPending = true;
        network.sendDrawOffer();
        statusLabel->setText("已向对方发出和棋申请，等待回应...");
        updateButtons();
        return;
    }

    QMessageBox::StandardButton ret = QMessageBox::question(
        this, "求和棋", "确定本局和棋吗？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        game.declareDraw();
        updateButtons();
    }
}

void MainWindow::enterReplayMode() {
    if (game.getMoveCount() <= 0) {
        return;
    }
    m_drawPending = false;
    stopMoveTimer();
    boardWidget->setReplayMode(true);
    boardWidget->setReplayStep(0);
    if (replayControls) {
        replayControls->show();
    }
    updateReplayStepLabel();
    updateButtons();
}

void MainWindow::leaveReplayMode() {
    boardWidget->setReplayMode(false);
    boardWidget->setReplayStep(-1);
    if (replayControls) {
        replayControls->hide();
    }
    updateMoveCount();
    updateButtons();
}

void MainWindow::onReplayClicked() {
    enterReplayMode();
}

void MainWindow::onReplayStepPrev() {
    if (!boardWidget || !boardWidget->isReplayMode()) {
        return;
    }
    int step = boardWidget->getReplayStep();
    if (step > 0) {
        boardWidget->setReplayStep(step - 1);
        updateReplayStepLabel();
    }
}

void MainWindow::onReplayStepNext() {
    if (!boardWidget || !boardWidget->isReplayMode()) {
        return;
    }
    int step = boardWidget->getReplayStep();
    int total = game.getMoveCount();
    if (step < total) {
        boardWidget->setReplayStep(step + 1);
        updateReplayStepLabel();
    }
}

void MainWindow::onReplayExit() {
    leaveReplayMode();
}

void MainWindow::recordGameResult(Gomoku::GameState state) {
    QSettings settings;
    settings.setValue("stats/games",
                      settings.value("stats/games", 0).toInt() + 1);

    if (state == Gomoku::GameState::BlackWin) {
        settings.setValue("stats/blackWins",
                          settings.value("stats/blackWins", 0).toInt() + 1);
    } else if (state == Gomoku::GameState::WhiteWin) {
        settings.setValue("stats/whiteWins",
                          settings.value("stats/whiteWins", 0).toInt() + 1);
    } else if (state == Gomoku::GameState::Draw) {
        settings.setValue("stats/draws",
                          settings.value("stats/draws", 0).toInt() + 1);
    }
    settings.sync();
}

void MainWindow::showStatsDialog() {
    QSettings settings;
    int blackWins = settings.value("stats/blackWins", 0).toInt();
    int whiteWins = settings.value("stats/whiteWins", 0).toInt();
    int draws = settings.value("stats/draws", 0).toInt();
    int games = settings.value("stats/games", 0).toInt();

    QString text;
    text += QString("总对局数：%1\n\n").arg(games);
    text += QString("黑方战绩：%1 胜 / %2 负 / %3 平\n")
                .arg(blackWins)
                .arg(whiteWins)
                .arg(draws);
    text += QString("白方战绩：%1 胜 / %2 负 / %3 平")
                .arg(whiteWins)
                .arg(blackWins)
                .arg(draws);

    QMessageBox::information(this, "战绩统计", text);
}

void MainWindow::onShowStats() {
    showStatsDialog();
}
