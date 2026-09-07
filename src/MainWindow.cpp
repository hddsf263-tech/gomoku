#include "MainWindow.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QFuture>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

#include <QtConcurrent/QtConcurrent>

#include "BoardWidget.h"
#include "SkinDialog.h"
#include "SoundManager.h"

namespace Gomoku {

namespace {

QString pieceText(Piece piece) {
    return piece == Piece::Black ? "黑方" : "白方";
}

QString statusTextFor(GameStatus status) {
    switch (status) {
    case GameStatus::BlackWin:
        return "黑方获胜";
    case GameStatus::WhiteWin:
        return "白方获胜";
    case GameStatus::Draw:
        return "平局";
    default:
        return "对局中";
    }
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , sounds_(new SoundManager(this))
{
    setWindowTitle("五子棋");
    resize(1180, 800);
    setMinimumSize(960, 700);

    network_.onConnected([this]() {
        const bool host = network_.role() == NetworkManager::Role::Host;
        myColor_ = host ? Piece::Black : Piece::White;
        netConnected_ = true;
        if (host) {
            network_.sendHello(0);
            showNetMessage("对手已加入，黑方先行", false);
            restartCurrent(false);
        } else {
            network_.sendHello(1);
            showNetMessage("已连接到主机，等待黑方落子", false);
            restartCurrent(false);
        }
    });
    network_.onDisconnected([this]() {
        netConnected_ = false;
        showNetMessage("连接已断开", true);
        setBoardInteraction();
        updateStatus();
    });
    network_.onError([this](const QString& message) {
        showNetMessage("网络错误：" + message, true);
    });
    network_.onMove([this](int row, int col) {
        if (netConnected_ && game_.status() == GameStatus::InProgress &&
            game_.currentPlayer() != myColor_) {
            doPlace(row, col, false, true);
        }
    });
    network_.onHello([this](int color) {
        myColor_ = color == 0 ? Piece::White : Piece::Black;
        updateStatus();
    });
    network_.onReset([this]() {
        restartCurrent(false);
    });

    setupUi();
    loadSettings();
    applySettings();
    setMode(Mode::HumanHuman);
    updateStatus();
}

MainWindow::~MainWindow() {
    cancelAi();
    network_.disconnectPeer();
}

void MainWindow::onModeChanged() {
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    central->setObjectName("root");
    central->setStyleSheet(
        "#root{background:#f2f1ec;}"
        "QFrame#card{background:#ffffff;border:1px solid #e2e1da;"
        "border-radius:8px;}"
        "QLabel#title{font-size:25px;font-weight:800;color:#1f2623;}"
        "QLabel#mutedLabel{color:#6b746f;font-size:13px;}"
        "QLabel#moveText{color:#1f2623;font-size:15px;font-weight:700;}"
        "QPushButton#modeHuman,QPushButton#modeAI,QPushButton#modeNet{"
        "height:42px;border:1px solid #d8d7cf;"
        "border-radius:8px;background:rgba(255,255,255,0.9);"
        "font-size:14px;font-weight:700;color:#6b746f;}"
        "QPushButton#modeHuman:checked,QPushButton#modeAI:checked,"
        "QPushButton#modeNet:checked{background:#2e5d52;border-color:#2e5d52;"
        "color:#ffffff;}"
        "QPushButton#actionBtn{height:44px;border:1px solid #d8d7cf;"
        "border-radius:8px;background:#ffffff;font-weight:700;color:#1f2623;}"
        "QPushButton#actionBtn:hover{background:#f7f6f1;}"
        "QPushButton#actionBtn:disabled{color:#9b9f9c;}"
        "QPushButton#primaryBtn{height:44px;border-radius:8px;"
        "background:#2e5d52;color:#ffffff;font-weight:700;border:none;}"
        "QPushButton#primaryBtn:hover{background:#38705f;}"
        "QPushButton#primaryBtn:disabled{background:#b7c6bf;color:#e8ece9;}"
        "QPushButton#headerBtn{height:38px;border:1px solid #d8d7cf;"
        "border-radius:8px;background:#ffffff;font-weight:700;color:#1f2623;"
        "padding:0 12px;}"
        "QPushButton#headerBtn:hover{background:#f7f6f1;}"
        "QLineEdit,QSpinBox{border:1px solid #d8d7cf;border-radius:7px;"
        "padding:0 8px;min-height:32px;background:#ffffff;}"
        "QComboBox{border:1px solid #d8d7cf;border-radius:7px;padding:0 8px;"
        "min-height:34px;background:#ffffff;}"
        "QRadioButton{color:#1f2623;font-weight:600;spacing:6px;}"
        "QScrollArea{border:none;background:transparent;}");
    setCentralWidget(central);

    auto* outer = new QVBoxLayout(central);
    outer->setContentsMargins(26, 22, 26, 24);
    outer->setSpacing(18);

    outer->addWidget(buildHeader());
    outer->addWidget(buildModeBar());

    auto* body = new QHBoxLayout;
    body->setSpacing(28);

    board_ = new BoardWidget(this);
    board_->setObjectName("board");
    board_->setGame(&game_);
    connect(board_, &BoardWidget::positionClicked,
            this, [this](int row, int col) {
        if (canHumanInput() && game_.canPlace(row, col)) {
            doPlace(row, col);
        }
    });
    body->addWidget(board_, 1);
    body->addWidget(buildSidebar(), 0);
    outer->addLayout(body, 1);
}

QWidget* MainWindow::buildHeader() {
    auto* header = new QWidget(this);
    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto* logo = new QLabel(header);
    logo->setPixmap(makeStonePixmap(-1, 42));
    logo->setFixedSize(42, 42);
    layout->addWidget(logo);

    auto* title = new QLabel("五子棋", header);
    title->setObjectName("title");
    layout->addWidget(title);
    layout->addStretch();

    auto* pill = new QFrame(header);
    pill->setObjectName("card");
    auto* pillLayout = new QHBoxLayout(pill);
    pillLayout->setContentsMargins(12, 7, 12, 7);
    pillLayout->setSpacing(8);
    statusDot_ = new QLabel(pill);
    statusDot_->setFixedSize(15, 15);
    statusText_ = new QLabel("黑方回合", pill);
    statusText_->setObjectName("statusText");
    statusText_->setStyleSheet("font-weight:700;color:#1f2623;");
    pillLayout->addWidget(statusDot_);
    pillLayout->addWidget(statusText_);
    layout->addWidget(pill);

    soundButton_ = new QPushButton("音效", header);
    soundButton_->setObjectName("headerBtn");
    soundButton_->setCursor(Qt::PointingHandCursor);
    connect(soundButton_, &QPushButton::clicked, this, &MainWindow::onSoundToggle);
    layout->addWidget(soundButton_);

    skinButton_ = new QPushButton("换肤", header);
    skinButton_->setObjectName("headerBtn");
    skinButton_->setCursor(Qt::PointingHandCursor);
    connect(skinButton_, &QPushButton::clicked, this, &MainWindow::onSkinDialog);
    layout->addWidget(skinButton_);

    return header;
}

QWidget* MainWindow::buildModeBar() {
    auto* bar = new QWidget(this);
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* group = new QButtonGroup(this);
    modeHuman_ = new QPushButton("双人", bar);
    modeAI_ = new QPushButton("人机", bar);
    modeNet_ = new QPushButton("联机", bar);
    for (QPushButton* button : { modeHuman_, modeAI_, modeNet_ }) {
        button->setObjectName(button == modeHuman_ ? "modeHuman"
                              : button == modeAI_ ? "modeAI"
                              : "modeNet");
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout->addWidget(button);
    }
    group->addButton(modeHuman_, 0);
    group->addButton(modeAI_, 1);
    group->addButton(modeNet_, 2);
    connect(group, &QButtonGroup::idClicked,
            this, [this](int id) {
        if (id == 0) {
            setMode(Mode::HumanHuman);
        } else if (id == 1) {
            setMode(Mode::HumanAI);
        } else {
            setMode(Mode::Network);
        }
    });
    return bar;
}

QWidget* MainWindow::buildSidebar() {
    auto* sidebar = new QWidget(this);
    sidebar->setFixedWidth(310);
    auto* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* players = new QWidget(sidebar);
    auto* playersLayout = new QHBoxLayout(players);
    playersLayout->setContentsMargins(0, 0, 0, 0);
    playersLayout->setSpacing(10);
    blackCard_ = buildPlayerCard(true, &blackName_, &blackTag_);
    whiteCard_ = buildPlayerCard(false, &whiteName_, &whiteTag_);
    playersLayout->addWidget(blackCard_);
    playersLayout->addWidget(whiteCard_);
    layout->addWidget(players);

    auto* actionRow = new QWidget(sidebar);
    auto* actionLayout = new QHBoxLayout(actionRow);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(10);
    undoButton_ = new QPushButton("悔棋", actionRow);
    restartButton_ = new QPushButton("重开", actionRow);
    undoButton_->setObjectName("actionBtn");
    restartButton_->setObjectName("primaryBtn");
    undoButton_->setCursor(Qt::PointingHandCursor);
    restartButton_->setCursor(Qt::PointingHandCursor);
    undoButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    restartButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(undoButton_, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(restartButton_, &QPushButton::clicked, this, &MainWindow::onRestart);
    actionLayout->addWidget(undoButton_);
    actionLayout->addWidget(restartButton_);
    layout->addWidget(actionRow);

    auto* moveCard = new QFrame(sidebar);
    moveCard->setObjectName("card");
    auto* moveLayout = new QVBoxLayout(moveCard);
    auto* moveLabel = new QLabel("当前手数", moveCard);
    moveLabel->setObjectName("mutedLabel");
    moveText_ = new QLabel("等待落子", moveCard);
    moveText_->setObjectName("moveText");
    moveLayout->addWidget(moveLabel);
    moveLayout->addWidget(moveText_);
    layout->addWidget(moveCard);

    // AI options
    aiOptions_ = new QWidget(sidebar);
    auto* aiLayout = new QVBoxLayout(aiOptions_);
    aiLayout->setContentsMargins(0, 0, 0, 0);
    aiLayout->setSpacing(10);
    auto* colorLabel = new QLabel("执子", aiOptions_);
    colorLabel->setObjectName("mutedLabel");
    aiLayout->addWidget(colorLabel);
    auto* colorRow = new QWidget(aiOptions_);
    auto* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    aiBlack_ = new QRadioButton("执黑", colorRow);
    aiWhite_ = new QRadioButton("执白", colorRow);
    aiBlack_->setChecked(true);
    auto* colorGroup = new QButtonGroup(colorRow);
    colorGroup->addButton(aiBlack_, 0);
    colorGroup->addButton(aiWhite_, 1);
    connect(colorGroup, &QButtonGroup::idClicked, this, [this](int id) {
        humanIsBlack_ = id == 0;
        if (mode_ == Mode::HumanAI) {
            restartCurrent(false);
        }
    });
    colorLayout->addWidget(aiBlack_);
    colorLayout->addWidget(aiWhite_);
    colorLayout->addStretch();
    aiLayout->addWidget(colorRow);
    auto* difficultyLabel = new QLabel("AI 难度", aiOptions_);
    difficultyLabel->setObjectName("mutedLabel");
    aiLayout->addWidget(difficultyLabel);
    difficultyCombo_ = new QComboBox(aiOptions_);
    difficultyCombo_->addItem("简单", "easy");
    difficultyCombo_->addItem("标准", "normal");
    difficultyCombo_->addItem("困难", "hard");
    difficultyCombo_->setCurrentIndex(1);
    connect(difficultyCombo_, &QComboBox::currentIndexChanged, this, [this]() {
        aiDifficulty_ = difficultyCombo_->currentData().toString();
        if (mode_ == Mode::HumanAI) {
            restartCurrent(false);
        }
    });
    aiLayout->addWidget(difficultyCombo_);
    aiOptions_->hide();

    // Network options
    netOptions_ = new QWidget(sidebar);
    auto* netLayout = new QVBoxLayout(netOptions_);
    netLayout->setContentsMargins(0, 0, 0, 0);
    netLayout->setSpacing(10);
    auto* roleRow = new QWidget(netOptions_);
    auto* roleLayout = new QHBoxLayout(roleRow);
    roleLayout->setContentsMargins(0, 0, 0, 0);
    netHost_ = new QRadioButton("创建房间", roleRow);
    netClient_ = new QRadioButton("加入房间", roleRow);
    netHost_->setChecked(true);
    auto* roleGroup = new QButtonGroup(roleRow);
    roleGroup->addButton(netHost_, 0);
    roleGroup->addButton(netClient_, 1);
    connect(roleGroup, &QButtonGroup::idClicked, roleRow,
            [this](int id) {
        netAddress_->setEnabled(id == 1);
        if (!netConnected_) {
            netAction_->setText(id == 0 ? "创建房间" : "加入房间");
        }
    });
    roleLayout->addWidget(netHost_);
    roleLayout->addWidget(netClient_);
    netLayout->addWidget(roleRow);
    auto* hostLabel = new QLabel("主机地址", netOptions_);
    hostLabel->setObjectName("mutedLabel");
    netLayout->addWidget(hostLabel);
    netAddress_ = new QLineEdit("127.0.0.1", netOptions_);
    netAddress_->setEnabled(false);
    netLayout->addWidget(netAddress_);
    auto* portLabel = new QLabel("端口", netOptions_);
    portLabel->setObjectName("mutedLabel");
    netLayout->addWidget(portLabel);
    netPort_ = new QSpinBox(netOptions_);
    netPort_->setRange(1, 65535);
    netPort_->setValue(12345);
    netLayout->addWidget(netPort_);
    netAction_ = new QPushButton("创建房间", netOptions_);
    netAction_->setObjectName("primaryBtn");
    netAction_->setCursor(Qt::PointingHandCursor);
    connect(netAction_, &QPushButton::clicked, this, &MainWindow::onNetAction);
    netLayout->addWidget(netAction_);
    netStatus_ = new QLabel("等待开始", netOptions_);
    netStatus_->setWordWrap(true);
    netLayout->addWidget(netStatus_);
    netOptions_->hide();

    layout->addWidget(aiOptions_);
    layout->addWidget(netOptions_);
    layout->addStretch();

    return sidebar;
}

QFrame* MainWindow::buildPlayerCard(bool black,
                                    QLabel** nameLabel,
                                    QLabel** tagLabel) {
    auto* card = new QFrame(this);
    card->setObjectName("card");
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(9);

    auto* icon = new QLabel(card);
    icon->setFixedSize(27, 27);
    icon->setPixmap(makeStonePixmap(black ? 0 : 1, 27));
    layout->addWidget(icon);
    if (black) {
        blackIcon_ = icon;
    } else {
        whiteIcon_ = icon;
    }

    auto* textColumn = new QVBoxLayout;
    textColumn->setSpacing(1);
    auto* name = new QLabel(black ? "黑方" : "白方", card);
    name->setStyleSheet("font-size:15px;font-weight:800;color:#1f2623;");
    auto* tag = new QLabel(black ? "先手" : "后手", card);
    tag->setStyleSheet("font-size:12px;color:#6b746f;");
    textColumn->addWidget(name);
    textColumn->addWidget(tag);
    layout->addLayout(textColumn);
    layout->addStretch();

    *nameLabel = name;
    *tagLabel = tag;
    return card;
}

QFrame* MainWindow::buildCard(const QString& title, QWidget* content) {
    Q_UNUSED(title);
    Q_UNUSED(content);
    return nullptr;
}

QPixmap MainWindow::makeStonePixmap(int piece, int size) const {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    if (piece == -1) {
        painter.setBrush(QColor(16, 18, 20));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(2, 4, size - 9, size - 9);
        painter.setBrush(QColor(255, 255, 255));
        painter.drawEllipse(size / 2, 2, size - 11, size - 11);
        return pixmap;
    }

    const bool black = piece == 0;
    const PiecePalette palette = Gomoku::piecePalette(settings_.pieceSkin);
    const QColor light = black ? palette.blackLight : palette.whiteLight;
    const QColor dark = black ? palette.blackDark : palette.whiteDark;
    QRadialGradient gradient(size / 3, size / 3, size * 0.75);
    gradient.setColorAt(0, light);
    gradient.setColorAt(1, dark);
    painter.setPen(QPen(black ? QColor(0, 0, 0, 80) : QColor(0, 0, 0, 30), 1));
    painter.setBrush(gradient);
    painter.drawEllipse(1, 1, size - 2, size - 2);
    return pixmap;
}

void MainWindow::setMode(Mode mode) {
    cancelAi();
    if (mode != Mode::Network && network_.isConnected()) {
        network_.disconnectPeer();
        netConnected_ = false;
    }
    mode_ = mode;

    modeHuman_->setChecked(mode == Mode::HumanHuman);
    modeAI_->setChecked(mode == Mode::HumanAI);
    modeNet_->setChecked(mode == Mode::Network);
    updateModePanel();

    if (mode == Mode::Network) {
        game_.reset();
        board_->clearGameVisuals();
        netStatus_->setText(netHost_->isChecked() ? "等待创建房间" : "等待加入房间");
        netAction_->setEnabled(true);
        netAction_->setText(netHost_->isChecked() ? "创建房间" : "加入房间");
    } else {
        restartCurrent(false);
    }
    setBoardInteraction();
    updateStatus();
}

void MainWindow::updateModePanel() {
    const bool ai = mode_ == Mode::HumanAI;
    const bool net = mode_ == Mode::Network;
    aiOptions_->setVisible(ai);
    netOptions_->setVisible(net);
}

void MainWindow::restartCurrent(bool notifyRemote) {
    cancelAi();
    if (notifyRemote && netConnected_) {
        network_.sendReset();
    }
    game_.reset();
    board_->clearGameVisuals();
    setBoardInteraction();
    updateStatus();

    if (mode_ == Mode::HumanAI && humanPiece() != Piece::Black) {
        scheduleAi();
    }
}

void MainWindow::doPlace(int row, int col, bool aiMove, bool remote) {
    if (!game_.canPlace(row, col)) {
        return;
    }
    const Piece placed = game_.currentPlayer();
    game_.makeMove(row, col);

    board_->playPlaceEffect(row, col, placed);
    sounds_->playPlace(placed);

    const GameStatus status = game_.status();
    if (status == GameStatus::BlackWin || status == GameStatus::WhiteWin) {
        const std::vector<GameMove> line = game_.winningLine(row, col);
        if (!line.empty() && settings_.winEffect != "none") {
            board_->playWinEffect(line);
        }
        sounds_->playWin();
    }

    if (!remote && mode_ == Mode::Network && netConnected_) {
        network_.sendMove(row, col);
    }

    updateStatus();
    if (mode_ == Mode::HumanAI && !aiMove &&
        game_.status() == GameStatus::InProgress &&
        game_.currentPlayer() == aiPiece()) {
        scheduleAi();
    }
}

void MainWindow::scheduleAi() {
    if (mode_ != Mode::HumanAI || aiThinking_ ||
        game_.status() != GameStatus::InProgress ||
        game_.currentPlayer() != aiPiece()) {
        return;
    }

    aiThinking_ = true;
    const int token = ++aiToken_;
    board_->setThinking(true);
    updateStatus();

    AiConfig config;
    if (aiDifficulty_ == "easy") {
        config = { 3, 500, 1000000 };
    } else if (aiDifficulty_ == "hard") {
        config = { 6, 5000, 1000000 };
    } else {
        config = { 4, 2000, 1000000 };
    }

    const GameEngine snapshot = game_;
    const Piece ai = aiPiece();
    auto* watcher = new QFutureWatcher<QPair<int, int>>(this);
    aiWatcher_ = watcher;
    connect(watcher, &QFutureWatcher<QPair<int, int>>::finished,
            this, [this, watcher, token]() {
        if (token != aiToken_) {
            watcher->deleteLater();
            if (aiWatcher_ == watcher) {
                aiWatcher_ = nullptr;
            }
            return;
        }
        const QPair<int, int> move = watcher->result();
        aiThinking_ = false;
        board_->setThinking(false);
        if (game_.status() == GameStatus::InProgress &&
            game_.currentPlayer() == aiPiece() &&
            game_.canPlace(move.first, move.second)) {
            doPlace(move.first, move.second, true, false);
        } else {
            updateStatus();
        }
        watcher->deleteLater();
        if (aiWatcher_ == watcher) {
            aiWatcher_ = nullptr;
        }
    });

    const QFuture<QPair<int, int>> future =
        QtConcurrent::run([snapshot, ai, config]() {
        const AiResult result = AiSearch::findBestMove(snapshot, ai, config);
        return QPair<int, int>(result.move.row, result.move.col);
    });
    watcher->setFuture(future);
}

void MainWindow::cancelAi() {
    aiToken_++;
    aiThinking_ = false;
    board_->setThinking(false);
    if (aiWatcher_) {
        aiWatcher_->cancel();
        aiWatcher_ = nullptr;
    }
}

void MainWindow::onAiFinished(int token) {
    Q_UNUSED(token);
}

void MainWindow::onUndo() {
    if (mode_ == Mode::Network || aiThinking_) {
        return;
    }
    const int steps = mode_ == Mode::HumanAI ? 2 : 1;
    for (int i = 0; i < steps; i++) {
        if (!game_.undo()) {
            break;
        }
    }
    board_->clearGameVisuals();
    updateStatus();
    if (mode_ == Mode::HumanAI &&
        game_.status() == GameStatus::InProgress &&
        game_.currentPlayer() == aiPiece()) {
        scheduleAi();
    }
}

void MainWindow::onRestart() {
    if (mode_ == Mode::Network && !netConnected_) {
        return;
    }
    restartCurrent(true);
}

void MainWindow::onSoundToggle() {
    SkinDialog dialog(&settings_, SkinDialog::Sound, this);
    connect(&dialog, &SkinDialog::applied, this, [this]() {
        applySettings();
        saveSettings();
    });
    dialog.exec();
}

void MainWindow::onSkinDialog() {
    SkinDialog dialog(&settings_, SkinDialog::Appearance, this);
    connect(&dialog, &SkinDialog::applied, this, [this]() {
        applySettings();
        saveSettings();
    });
    dialog.exec();
}

void MainWindow::onNetAction() {
    if (netConnected_) {
        stopNetwork();
    } else {
        startNetwork();
    }
}

void MainWindow::startNetwork() {
    cancelAi();
    game_.reset();
    board_->clearGameVisuals();

    const bool host = netHost_->isChecked();
    const int port = netPort_->value();
    if (host) {
        myColor_ = Piece::Black;
        if (!network_.listen(static_cast<quint16>(port))) {
            showNetMessage("端口监听失败，请更换端口", true);
            return;
        }
        showNetMessage("已开启房间，等待对手加入", false);
        netAction_->setText("断开连接");
    } else {
        myColor_ = Piece::White;
        const QString address = netAddress_->text().trimmed();
        network_.connectToHost(address, static_cast<quint16>(port));
        showNetMessage("正在连接主机...", false);
        netAction_->setText("断开连接");
    }
    setBoardInteraction();
    updateStatus();
}

void MainWindow::stopNetwork() {
    network_.disconnectPeer();
    netConnected_ = false;
    showNetMessage("已断开连接", false);
    netAction_->setText(netHost_->isChecked() ? "创建房间" : "加入房间");
    game_.reset();
    board_->clearGameVisuals();
    setBoardInteraction();
    updateStatus();
}

bool MainWindow::canHumanInput() const {
    if (aiThinking_ || game_.status() != GameStatus::InProgress) {
        return false;
    }
    if (mode_ == Mode::HumanAI) {
        return game_.currentPlayer() == humanPiece();
    }
    if (mode_ == Mode::Network) {
        return netConnected_ && game_.currentPlayer() == myColor_;
    }
    return true;
}

void MainWindow::setBoardInteraction() {
    board_->setGhostAllowed(canHumanInput());
    board_->setThinking(aiThinking_);
}

void MainWindow::showNetMessage(const QString& text, bool error) {
    netStatus_->setText(text);
    netStatus_->setStyleSheet(error
        ? "color:#b3453a;font-weight:600;"
        : "color:#2d8a64;font-weight:600;");
}

void MainWindow::applySettings() {
    const BoardPalette boardPalette = Gomoku::boardPalette(settings_.boardSkin);
    board_->setBoardColors(boardPalette.base, boardPalette.line, boardPalette.star);
    board_->setBoardImage(settings_.boardImage);
    board_->setEffectModes(settings_.placeEffect, settings_.winEffect);

    const PiecePalette palette = Gomoku::piecePalette(settings_.pieceSkin);
    board_->setPieceGradient(0, palette.blackLight, palette.blackDark);
    board_->setPieceGradient(1, palette.whiteLight, palette.whiteDark);
    board_->setPieceImages(settings_.blackImage, settings_.whiteImage);

    sounds_->setMuted(settings_.muted);
    sounds_->setPlaceSound(settings_.placeSound, settings_.customPlaceAudio);
    sounds_->setWinSound(settings_.winSound, settings_.customWinAudio);
    setSoundButtonUi();

    if (blackIcon_) {
        blackIcon_->setPixmap(makeStonePixmap(0, 27));
    }
    if (whiteIcon_) {
        whiteIcon_->setPixmap(makeStonePixmap(1, 27));
    }
    board_->update();
}

void MainWindow::loadSettings() {
    QSettings settings;
    settings.beginGroup("gomoku");
    settings_.boardSkin = settings.value("boardSkin", "jade").toString();
    settings_.pieceSkin = settings.value("pieceSkin", "classic").toString();
    settings_.placeEffect = settings.value("placeEffect", "ring").toString();
    settings_.winEffect = settings.value("winEffect", "pulse").toString();
    settings_.placeSound = settings.value("placeSound", "wood").toString();
    settings_.winSound = settings.value("winSound", "chord").toString();
    settings_.boardImage = settings.value("boardImage").toString();
    settings_.blackImage = settings.value("blackImage").toString();
    settings_.whiteImage = settings.value("whiteImage").toString();
    settings_.customPlaceAudio = settings.value("customPlaceAudio").toString();
    settings_.customWinAudio = settings.value("customWinAudio").toString();
    settings_.muted = settings.value("muted", false).toBool();
    humanIsBlack_ = settings.value("humanBlack", true).toBool();
    aiDifficulty_ = settings.value("aiDifficulty", "normal").toString();
    settings.endGroup();

    if (aiBlack_ && aiWhite_) {
        aiBlack_->setChecked(humanIsBlack_);
        aiWhite_->setChecked(!humanIsBlack_);
    }
    if (difficultyCombo_) {
        const int index = difficultyCombo_->findData(aiDifficulty_);
        if (index >= 0) {
            difficultyCombo_->setCurrentIndex(index);
        }
    }
}

void MainWindow::saveSettings() const {
    QSettings settings;
    settings.beginGroup("gomoku");
    settings.setValue("boardSkin", settings_.boardSkin);
    settings.setValue("pieceSkin", settings_.pieceSkin);
    settings.setValue("placeEffect", settings_.placeEffect);
    settings.setValue("winEffect", settings_.winEffect);
    settings.setValue("placeSound", settings_.placeSound);
    settings.setValue("winSound", settings_.winSound);
    settings.setValue("boardImage", settings_.boardImage);
    settings.setValue("blackImage", settings_.blackImage);
    settings.setValue("whiteImage", settings_.whiteImage);
    settings.setValue("customPlaceAudio", settings_.customPlaceAudio);
    settings.setValue("customWinAudio", settings_.customWinAudio);
    settings.setValue("muted", settings_.muted);
    settings.setValue("humanBlack", humanIsBlack_);
    settings.setValue("aiDifficulty", aiDifficulty_);
    settings.endGroup();
}

void MainWindow::setSoundButtonUi() {
    soundButton_->setText(settings_.muted ? "音效关" : "音效开");
}

void MainWindow::updateStatus() {
    QString text;
    Piece activeColor = game_.currentPlayer();

    if (game_.status() == GameStatus::BlackWin ||
        game_.status() == GameStatus::WhiteWin) {
        const Piece winner = game_.status() == GameStatus::BlackWin
            ? Piece::Black
            : Piece::White;
        activeColor = winner;
        if (mode_ == Mode::HumanAI) {
            text = winner == humanPiece() ? "你获胜" : "AI 获胜";
        } else if (mode_ == Mode::Network) {
            text = winner == myColor_ ? "你赢了" : "对方获胜";
        } else {
            text = pieceText(winner) + "获胜";
        }
    } else if (game_.status() == GameStatus::Draw) {
        text = "平局";
    } else if (aiThinking_) {
        text = "AI 思考中";
        activeColor = aiPiece();
    } else if (mode_ == Mode::HumanAI) {
        text = game_.currentPlayer() == humanPiece() ? "你的回合" : "AI 回合";
        activeColor = game_.currentPlayer();
    } else if (mode_ == Mode::Network) {
        if (!netConnected_) {
            text = netHost_->isChecked() ? "等待对手加入" : "等待主机";
        } else {
            text = game_.currentPlayer() == myColor_ ? "你的回合" : "对方回合";
        }
        activeColor = netConnected_ ? game_.currentPlayer() : Piece::Black;
    } else {
        text = pieceText(game_.currentPlayer()) + "回合";
        activeColor = game_.currentPlayer();
    }

    statusText_->setText(text);
    const bool activeIsBlack = activeColor == Piece::Black;
    statusDot_->setStyleSheet(QString(
        "background:%1;border-radius:7px;border:1px solid %2;")
        .arg(activeIsBlack ? "#101214" : "#ffffff",
             activeIsBlack ? "#ffffff" : "#d4d0c3"));

    const auto last = game_.lastMove();
    moveText_->setText(last.has_value()
        ? QString::number(game_.moveCount()) + " 手 · " +
          QChar('A' + last->col) + QString::number(last->row + 1)
        : "等待落子");

    const bool blackActive = activeColor == Piece::Black;
    blackCard_->setProperty("active", blackActive);
    whiteCard_->setProperty("active", !blackActive);
    for (QFrame* card : { blackCard_, whiteCard_ }) {
        card->style()->unpolish(card);
        card->style()->polish(card);
    }

    QString blackName = "黑方";
    QString blackTag = "先手";
    QString whiteName = "白方";
    QString whiteTag = "后手";
    if (mode_ == Mode::HumanAI) {
        const bool humanBlack = humanPiece() == Piece::Black;
        blackName = humanBlack ? "你" : "AI";
        blackTag = humanBlack ? "你 · 先手" : "AI · 先手";
        whiteName = humanBlack ? "AI" : "你";
        whiteTag = humanBlack ? "AI · 后手" : "你 · 后手";
    } else if (mode_ == Mode::Network) {
        const bool host = network_.role() == NetworkManager::Role::Host;
        blackName = myColor_ == Piece::Black ? "我" : "对方";
        whiteName = myColor_ == Piece::White ? "我" : "对方";
        blackTag = host ? "主机" : "客户端";
        whiteTag = host ? "主机" : "客户端";
    }
    blackName_->setText(blackName);
    blackTag_->setText(blackTag);
    whiteName_->setText(whiteName);
    whiteTag_->setText(whiteTag);

    undoButton_->setEnabled(!aiThinking_ &&
        mode_ != Mode::Network && game_.moveCount() > 0);
    setBoardInteraction();
}

} // namespace Gomoku
