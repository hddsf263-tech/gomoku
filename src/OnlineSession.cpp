#include "OnlineSession.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>

#include "NetLink.h"
#include "NetProtocol.h"

namespace Gomoku {

namespace {

constexpr int kHeartbeatIntervalMs = 3000;
constexpr int kHeartbeatTimeoutMs = 12000;
constexpr int kConnectTimeoutMs = 8000;
constexpr int kTimerTickMs = 500;

QString statusName(GameStatus status) {
    switch (status) {
    case GameStatus::BlackWin: return QStringLiteral("BLACK_WIN");
    case GameStatus::WhiteWin: return QStringLiteral("WHITE_WIN");
    case GameStatus::Draw:     return QStringLiteral("DRAW");
    default:                   return QStringLiteral("IN_PROGRESS");
    }
}

QString winnerName(GameStatus status) {
    switch (status) {
    case GameStatus::BlackWin: return QStringLiteral("BLACK");
    case GameStatus::WhiteWin: return QStringLiteral("WHITE");
    case GameStatus::Draw:     return QStringLiteral("DRAW");
    default:                   return QStringLiteral("NONE");
    }
}

QString pieceName(Piece p) {
    return p == Piece::Black ? QStringLiteral("BLACK") : QStringLiteral("WHITE");
}

} // namespace

QString onlineStateName(OnlineState state) {
    switch (state) {
    case OnlineState::Disconnected:         return QStringLiteral("Disconnected");
    case OnlineState::Connecting:           return QStringLiteral("Connecting");
    case OnlineState::WaitingOpponent:      return QStringLiteral("WaitingOpponent");
    case OnlineState::Playing:              return QStringLiteral("Playing");
    case OnlineState::GameOver:             return QStringLiteral("GameOver");
    case OnlineState::Restarting:           return QStringLiteral("Restarting");
    case OnlineState::OpponentDisconnected: return QStringLiteral("OpponentDisconnected");
    case OnlineState::Error:                return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

OnlineSession::OnlineSession(GameEngine& game, QObject* parent)
    : QObject(parent)
    , game_(game)
{
    heartbeatTimer_.setInterval(kHeartbeatIntervalMs);
    connect(&heartbeatTimer_, &QTimer::timeout, this, &OnlineSession::onHeartbeatTick);
    connectTimeoutTimer_.setInterval(kConnectTimeoutMs);
    connectTimeoutTimer_.setSingleShot(true);
    connect(&connectTimeoutTimer_, &QTimer::timeout, this, &OnlineSession::onConnectTimeout);
    tickTimer_.setInterval(kTimerTickMs);
    connect(&tickTimer_, &QTimer::timeout, this, &OnlineSession::onTimerTick);
}

OnlineSession::~OnlineSession() {
    leaving_ = true;
    if (link_) {
        link_->disconnectPeer();
    }
}
void OnlineSession::setTimeLimitMs(qint64 ms) {
    timeLimitMs_ = ms >= 0 ? ms : 0;
    if (state_ != OnlineState::Playing) {
        resetTimers();
    }
}

bool OnlineSession::isHost() const {
    return link_ && link_->role() == net::NetLink::Role::Host;
}

bool OnlineSession::isConnected() const {
    return link_ && link_->isConnected();
}

bool OnlineSession::startHost(quint16 port) {
    leaving_ = false;
    stopHeartbeat();
    stopTimerTick();
    connectTimeoutTimer_.stop();
    if (link_) {
        link_->disconnectPeer();
    }
    myColor_ = Piece::Black;
    port_ = port;
    wantRematch_ = false;
    oppRematch_ = false;
    game_.reset();
    resetTimers();

    link_ = std::make_unique<net::NetLink>(this);
    connect(link_.get(), &net::NetLink::connected, this, &OnlineSession::onLinkConnected);
    connect(link_.get(), &net::NetLink::disconnected, this, &OnlineSession::onLinkDisconnected);
    connect(link_.get(), &net::NetLink::errorOccurred, this, &OnlineSession::onLinkError);
    connect(link_.get(), &net::NetLink::messageReceived, this, &OnlineSession::onMessage);

    if (!link_->listen(port)) {
        setState(OnlineState::Error);
        emit errorOccurred(QStringLiteral("端口监听失败，可能是端口被占用"));
        emit logMessage(logPrefix() + QStringLiteral("listen failed on port %1").arg(port));
        return false;
    }
    port_ = link_->port();
    setState(OnlineState::WaitingOpponent);
    emit logMessage(logPrefix() + QStringLiteral("Server started, listening on port %1").arg(port_));
    return true;
}

void OnlineSession::connectToHost(const QString& host, quint16 port) {
    leaving_ = false;
    stopHeartbeat();
    stopTimerTick();
    connectTimeoutTimer_.stop();
    if (link_) {
        link_->disconnectPeer();
    }
    myColor_ = Piece::White;
    hostAddress_ = host;
    port_ = port;
    wantRematch_ = false;
    oppRematch_ = false;
    game_.reset();
    resetTimers();

    link_ = std::make_unique<net::NetLink>(this);
    connect(link_.get(), &net::NetLink::connected, this, &OnlineSession::onLinkConnected);
    connect(link_.get(), &net::NetLink::disconnected, this, &OnlineSession::onLinkDisconnected);
    connect(link_.get(), &net::NetLink::errorOccurred, this, &OnlineSession::onLinkError);
    connect(link_.get(), &net::NetLink::messageReceived, this, &OnlineSession::onMessage);

    setState(OnlineState::Connecting);
    connectTimeoutTimer_.start();
    emit logMessage(logPrefix() + QStringLiteral("Connecting to %1:%2").arg(host).arg(port));
    link_->connectToHost(host, port);
}
void OnlineSession::leaveSession() {
    leaving_ = true;
    stopHeartbeat();
    stopTimerTick();
    connectTimeoutTimer_.stop();
    if (link_) {
        send(net::makeMessage(net::kTypeBye));
        link_->disconnectPeer();
    }
    game_.reset();
    resetTimers();
    wantRematch_ = false;
    oppRematch_ = false;
    setState(OnlineState::Disconnected);
    emit logMessage(logPrefix() + QStringLiteral("Session closed by user"));
    leaving_ = false;
}

void OnlineSession::setState(OnlineState s) {
    if (state_ == s) {
        return;
    }
    state_ = s;
    emit stateChanged(s);
    emit logMessage(logPrefix() + QStringLiteral("state=%1").arg(onlineStateName(s)));
}

void OnlineSession::send(const QJsonObject& obj) {
    if (link_ && link_->isConnected()) {
        link_->send(obj);
    }
}

// ---------------------------------------------------------------------------
// 网络事件
// ---------------------------------------------------------------------------

void OnlineSession::onLinkConnected() {
    if (!link_) {
        return;
    }
    connectTimeoutTimer_.stop();
    if (isHost()) {
        hostOnPeerJoined();
    } else {
        clientSendJoin();
    }
}

void OnlineSession::onLinkDisconnected() {
    if (leaving_) {
        return;
    }
    if (state_ == OnlineState::Disconnected || state_ == OnlineState::Error) {
        return;
    }
    handlePeerGone();
}

void OnlineSession::onLinkError(const QString& text) {
    emit errorOccurred(text);
    emit logMessage(logPrefix() + QStringLiteral("socket error: %1").arg(text));
    if (state_ == OnlineState::Connecting) {
        connectTimeoutTimer_.stop();
        if (link_) {
            link_->disconnectPeer();
        }
        setState(OnlineState::Disconnected);
        emit errorOccurred(QStringLiteral("连接失败：无法连接到主机"));
    } else if (state_ == OnlineState::Playing ||
               state_ == OnlineState::GameOver ||
               state_ == OnlineState::WaitingOpponent) {
        handlePeerGone();
    }
}
void OnlineSession::onMessage(const QJsonObject& obj) {
    if (!link_) {
        return;
    }
    lastActivityMs_ = QDateTime::currentMSecsSinceEpoch();

    const QString type = net::typeOf(obj);
    if (type == net::kTypePing) {
        send(net::makeMessage(net::kTypePong));
        return;
    }
    if (type == net::kTypePong) {
        return;
    }
    if (type == net::kTypeJoin) {
        return;
    }
    if (type == net::kTypeWelcome) {
        clientApplyWelcome(obj);
        return;
    }
    if (type == net::kTypeState) {
        if (!isHost()) {
            clientApplyState(obj);
        }
        return;
    }
    if (type == net::kTypeReqMove) {
        if (isHost()) {
            const int col = obj.value(QStringLiteral("x")).toInt(-1);
            const int row = obj.value(QStringLiteral("y")).toInt(-1);
            const Piece clientColor = myColor_ == Piece::Black ? Piece::White : Piece::Black;
            QString reason;
            if (!hostValidateMove(row, col, clientColor, reason)) {
                send(net::makeMessage(net::kTypeReject,
                                      QJsonObject{{QStringLiteral("reason"), reason}}));
                emit logMessage(logPrefix() + QStringLiteral("MOVE rejected: %1").arg(reason));
                return;
            }
            hostCommitMove(row, col, clientColor);
        }
        return;
    }
    if (type == net::kTypeMove) {
        if (!isHost()) {
            clientApplyMove(obj);
        }
        return;
    }
    if (type == net::kTypeReject) {
        if (!isHost()) {
            const QString reason = obj.value(QStringLiteral("reason")).toString();
            emit moveRejected(reason);
            emit logMessage(logPrefix() + QStringLiteral("MOVE rejected by host: %1").arg(reason));
        }
        return;
    }
    if (type == net::kTypeGameOver) {
        if (!isHost()) {
            clientApplyGameOver(obj);
        }
        return;
    }
    if (type == net::kTypeNewGame) {
        if (!isHost()) {
            clientApplyNewGame(obj);
        }
        return;
    }
    if (type == net::kTypeRematch) {
        handleRematch(obj);
        return;
    }
    if (type == net::kTypeResign) {
        const Piece fallback = isHost() ? Piece::White : Piece::Black;
        const Piece resigner = static_cast<Piece>(
            obj.value(QStringLiteral("resigner")).toInt(static_cast<int>(fallback)));
        handleResign(obj, resigner);
        return;
    }
    if (type == net::kTypeChat) {
        if (isHost()) {
            handleChat(obj);
        } else {
            applyChat(obj);
        }
        return;
    }
    if (type == net::kTypeBye) {
        handlePeerGone();
        return;
    }

    emit logMessage(logPrefix() + QStringLiteral("unknown message type: %1").arg(type));
}
// ---------------------------------------------------------------------------
// Host 逻辑
// ---------------------------------------------------------------------------

void OnlineSession::hostOnPeerJoined() {
    game_.reset();
    myColor_ = Piece::Black;
    emit colorAssigned(Piece::Black);

    setState(OnlineState::Playing);
    resetTimers();
    beginTurn();
    send(net::makeMessage(net::kTypeWelcome,
                          QJsonObject{{QStringLiteral("color"), static_cast<int>(Piece::White)}}));
    hostBroadcastState();
    emit sessionStarted();
    startHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("Client connected, black first"));
}

bool OnlineSession::hostValidateMove(int row, int col, Piece player, QString& reason) const {
    if (state_ != OnlineState::Playing) {
        reason = QStringLiteral("GAME_NOT_ACTIVE");
        return false;
    }
    if (row < 0 || row >= 15 || col < 0 || col >= 15) {
        reason = QStringLiteral("INVALID_POSITION");
        return false;
    }
    if (game_.currentPlayer() != player) {
        reason = QStringLiteral("NOT_YOUR_TURN");
        return false;
    }
    if (game_.status() != GameStatus::InProgress) {
        reason = QStringLiteral("GAME_OVER");
        return false;
    }
    if (!game_.canPlace(row, col)) {
        reason = QStringLiteral("CELL_OCCUPIED");
        return false;
    }
    if (timeLimitMs_ > 0 && turnStartMs_ > 0 &&
        activeRemainingMs_ - (QDateTime::currentMSecsSinceEpoch() - turnStartMs_) <= 0) {
        reason = QStringLiteral("TIME_EXPIRED");
        return false;
    }
    return true;
}

void OnlineSession::hostCommitMove(int row, int col, Piece player) {
    Q_UNUSED(player);
    settleTurn(game_.currentPlayer());
    const Piece placed = game_.currentPlayer();
    if (!game_.makeMove(row, col)) {
        emit errorOccurred(QStringLiteral("主机校验与落子不一致"));
        return;
    }
    send(net::makeMessage(net::kTypeMove,
                          QJsonObject{{QStringLiteral("x"), col},
                                      {QStringLiteral("y"), row},
                                      {QStringLiteral("piece"), static_cast<int>(placed)},
                                      {QStringLiteral("blackMs"), blackRemainingMs_},
                                      {QStringLiteral("whiteMs"), whiteRemainingMs_}}));
    emit moveCommitted(row, col, placed, game_.status());
    emit logMessage(logPrefix() + QStringLiteral("MOVE accepted x=%1 y=%2 player=%3")
                        .arg(col).arg(row).arg(placed == Piece::Black ? "BLACK" : "WHITE"));
    checkGameOverAfterMove(row, col);
}

void OnlineSession::hostBroadcastState() {
    QJsonArray moves;
    const auto& hist = game_.history();
    for (const GameMove& m : hist) {
        moves.append(QJsonArray({m.col, m.row}));
    }
    QJsonObject data;
    data[QStringLiteral("moves")] = moves;
    data[QStringLiteral("current")] = static_cast<int>(game_.currentPlayer());
    data[QStringLiteral("status")] = statusName(game_.status());
    data[QStringLiteral("winner")] = winnerName(game_.status());
    data[QStringLiteral("blackMs")] = blackRemainingMs_;
    data[QStringLiteral("whiteMs")] = whiteRemainingMs_;
    data[QStringLiteral("turnStart")] = turnStartMs_;
    data[QStringLiteral("timeLimit")] = timeLimitMs_;
    send(net::makeMessage(net::kTypeState, data));
}

void OnlineSession::hostStartNewGame() {
    game_.reset();
    resetTimers();
    setState(OnlineState::Playing);
    beginTurn();
    send(net::makeMessage(net::kTypeNewGame,
                          QJsonObject{{QStringLiteral("blackMs"), blackRemainingMs_},
                                      {QStringLiteral("whiteMs"), whiteRemainingMs_},
                                      {QStringLiteral("timeLimit"), timeLimitMs_}}));
    emit sessionStarted();
    emit rematchAccepted();
    startHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("New game started (rematch)"));
}

void OnlineSession::maybeStartRematch() {
    if (isHost() && wantRematch_ && oppRematch_) {
        hostStartNewGame();
    }
}
// ---------------------------------------------------------------------------
// Client 逻辑
// ---------------------------------------------------------------------------

void OnlineSession::clientSendJoin() {
    send(net::makeMessage(net::kTypeJoin));
    emit logMessage(logPrefix() + QStringLiteral("JOIN sent"));
}

void OnlineSession::clientApplyWelcome(const QJsonObject& data) {
    const int color = data.value(QStringLiteral("color")).toInt(static_cast<int>(Piece::White));
    myColor_ = static_cast<Piece>(color);
    emit colorAssigned(myColor_);
}

void OnlineSession::clientApplyState(const QJsonObject& data) {
    resetGame();
    const QJsonArray moves = data.value(QStringLiteral("moves")).toArray();

    int lastRow = -1;
    int lastCol = -1;
    for (int i = 0; i < moves.size(); ++i) {
        const QJsonArray m = moves.at(i).toArray();
        if (m.size() < 2) {
            continue;
        }
        const int col = m.at(0).toInt(-1);
        const int row = m.at(1).toInt(-1);
        if (row >= 0 && row < 15 && col >= 0 && col < 15 && game_.canPlace(row, col)) {
            game_.makeMove(row, col);
            lastRow = row;
            lastCol = col;
        }
    }

    const qint64 blackMs = data.value(QStringLiteral("blackMs")).toInt(0);
    const qint64 whiteMs = data.value(QStringLiteral("whiteMs")).toInt(0);
    const Piece current = static_cast<Piece>(
        data.value(QStringLiteral("current")).toInt(static_cast<int>(game_.currentPlayer())));
    blackRemainingMs_ = blackMs;
    whiteRemainingMs_ = whiteMs;
    timeLimitMs_ = static_cast<qint64>(data.value(QStringLiteral("timeLimit")).toDouble(timeLimitMs_));

    setState(OnlineState::Playing);
    emit sessionStarted();
    startHeartbeat();

    if (lastRow >= 0 && lastCol >= 0) {
        const auto last = game_.lastMove();
        Piece piece = Piece::Black;
        if (last.has_value()) {
            piece = last->piece;
        }
        const GameStatus status = game_.status();
        emit moveCommitted(lastRow, lastCol, piece, status);
        if (status != GameStatus::InProgress) {
            const std::vector<GameMove> line = game_.winningLine(lastRow, lastCol);
            emit gameStatusChanged(status, line, true);
            stopHeartbeat();
            stopTimerTick();
            emit timeUpdated(blackRemainingMs_, whiteRemainingMs_, current);
        } else {
            beginTurn();
        }
    } else {
        beginTurn();
    }
    emit logMessage(logPrefix() + QStringLiteral("STATE applied (%1 moves)").arg(moves.size()));
}
void OnlineSession::clientApplyMove(const QJsonObject& data) {
    const int col = data.value(QStringLiteral("x")).toInt(-1);
    const int row = data.value(QStringLiteral("y")).toInt(-1);
    if (row < 0 || row >= 15 || col < 0 || col >= 15 || !game_.canPlace(row, col)) {
        emit errorOccurred(QStringLiteral("收到与本地棋局不一致的落子"));
        emit logMessage(logPrefix() + QStringLiteral("MOVE out of sync x=%1 y=%2").arg(col).arg(row));
        return;
    }
    const Piece placed = game_.currentPlayer();
    game_.makeMove(row, col);

    const qint64 blackMs = data.value(QStringLiteral("blackMs")).toInt(-1);
    const qint64 whiteMs = data.value(QStringLiteral("whiteMs")).toInt(-1);
    if (blackMs >= 0) {
        blackRemainingMs_ = blackMs;
    }
    if (whiteMs >= 0) {
        whiteRemainingMs_ = whiteMs;
    }

    emit moveCommitted(row, col, placed, game_.status());
    emit logMessage(logPrefix() + QStringLiteral("MOVE applied x=%1 y=%2").arg(col).arg(row));
    if (game_.status() != GameStatus::InProgress) {
        setState(OnlineState::GameOver);
        const std::vector<GameMove> line = game_.winningLine(row, col);
        emit gameStatusChanged(game_.status(), line, true);
        stopHeartbeat();
        stopTimerTick();
        emit timeUpdated(blackRemainingMs_, whiteRemainingMs_, game_.currentPlayer());
        emit logMessage(logPrefix() + QStringLiteral("GAME_OVER winner=%1")
                            .arg(winnerName(game_.status())));
    } else {
        beginTurn();
    }
}

void OnlineSession::clientApplyGameOver(const QJsonObject& data) {
    if (state_ == OnlineState::GameOver) {
        return;
    }
    const QString winner = data.value(QStringLiteral("winner")).toString();
    GameStatus status = GameStatus::Draw;
    if (winner == QStringLiteral("BLACK")) {
        status = GameStatus::BlackWin;
    } else if (winner == QStringLiteral("WHITE")) {
        status = GameStatus::WhiteWin;
    }
    if (status != GameStatus::Draw) {
        game_.forceResult(status);
    }
    setState(OnlineState::GameOver);
    stopTimerTick();
    const auto last = game_.lastMove();
    std::vector<GameMove> line;
    if (last.has_value() && status != GameStatus::Draw) {
        line = game_.winningLine(last->row, last->col);
    }
    emit gameStatusChanged(status, line, true);
    stopHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("GAME_OVER (explicit) winner=%1").arg(winner));
}

void OnlineSession::clientApplyNewGame(const QJsonObject& data) {
    resetGame();
    const qint64 blackMs = data.value(QStringLiteral("blackMs")).toInt(-1);
    const qint64 whiteMs = data.value(QStringLiteral("whiteMs")).toInt(-1);
    if (blackMs >= 0) {
        blackRemainingMs_ = blackMs;
    }
    if (whiteMs >= 0) {
        whiteRemainingMs_ = whiteMs;
    }
    timeLimitMs_ = static_cast<qint64>(data.value(QStringLiteral("timeLimit")).toDouble(timeLimitMs_));
    setState(OnlineState::Playing);
    emit sessionStarted();
    emit rematchAccepted();
    startHeartbeat();
    beginTurn();
    emit logMessage(logPrefix() + QStringLiteral("NEW_GAME applied (rematch)"));
}
// ---------------------------------------------------------------------------
// 重赛
// ---------------------------------------------------------------------------

void OnlineSession::handleRematch(const QJsonObject& data) {
    if (state_ != OnlineState::GameOver && state_ != OnlineState::Restarting) {
        return;
    }
    const bool accept = data.value(QStringLiteral("accept")).toBool(true);
    oppRematch_ = accept;
    if (accept) {
        if (!wantRematch_) {
            emit rematchRequested();
            return;
        }
        if (state_ != OnlineState::Restarting) {
            setState(OnlineState::Restarting);
            startHeartbeat();
        }
        if (isHost() && wantRematch_) {
            hostStartNewGame();
        }
    } else {
        if (wantRematch_) {
            wantRematch_ = false;
            setState(OnlineState::GameOver);
            stopHeartbeat();
        }
        emit rematchDeclined();
    }
}

void OnlineSession::handleResign(const QJsonObject& data, const Piece resigner) {
    Q_UNUSED(data);
    if (state_ != OnlineState::Playing) {
        return;
    }
    const Piece opponent = myColor_ == Piece::Black ? Piece::White : Piece::Black;
    if (resigner != opponent) {
        emit logMessage(logPrefix() + QStringLiteral("ignored RESIGN from unexpected party"));
        return;
    }
    const GameStatus status = resigner == Piece::Black ? GameStatus::WhiteWin : GameStatus::BlackWin;
    game_.forceResult(status);
    setState(OnlineState::GameOver);
    stopTimerTick();
    emit resigned(resigner, status);
    emit gameStatusChanged(status, {}, true);
    stopHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("opponent RESIGN -> %1")
                        .arg(status == GameStatus::WhiteWin ? "WHITE_WIN" : "BLACK_WIN"));
}

// ---------------------------------------------------------------------------
// UI 交互
// ---------------------------------------------------------------------------

bool OnlineSession::localMove(int row, int col) {
    if (state_ != OnlineState::Playing) {
        return false;
    }
    if (game_.status() != GameStatus::InProgress) {
        return false;
    }
    if (game_.currentPlayer() != myColor_) {
        return false;
    }
    if (isHost()) {
        QString reason;
        if (!hostValidateMove(row, col, myColor_, reason)) {
            emit logMessage(logPrefix() + QStringLiteral("local host move rejected: %1").arg(reason));
            return false;
        }
        hostCommitMove(row, col, myColor_);
        return true;
    }
    send(net::makeMessage(net::kTypeReqMove,
                          QJsonObject{{QStringLiteral("x"), col},
                                      {QStringLiteral("y"), row}}));
    emit logMessage(logPrefix() + QStringLiteral("REQ_MOVE sent x=%1 y=%2").arg(col).arg(row));
    return true;
}

void OnlineSession::requestRematch() {
    if (state_ != OnlineState::GameOver) {
        return;
    }
    wantRematch_ = true;
    setState(OnlineState::Restarting);
    startHeartbeat();
    send(net::makeMessage(net::kTypeRematch, QJsonObject{{QStringLiteral("accept"), true}}));
    emit logMessage(logPrefix() + QStringLiteral("REMATCH requested"));
    maybeStartRematch();
}

void OnlineSession::resign() {
    if (state_ != OnlineState::Playing) {
        return;
    }
    stopTimerTick();
    const GameStatus status = myColor_ == Piece::Black ? GameStatus::WhiteWin : GameStatus::BlackWin;
    game_.forceResult(status);
    setState(OnlineState::GameOver);
    emit resigned(myColor_, status);
    emit gameStatusChanged(status, {}, true);
    send(net::makeMessage(net::kTypeResign,
                          QJsonObject{{QStringLiteral("resigner"), static_cast<int>(myColor_)}}));
    stopHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("RESIGN sent"));
}

void OnlineSession::answerRematch(bool accept) {
    if (state_ != OnlineState::GameOver && state_ != OnlineState::Restarting) {
        return;
    }
    oppRematch_ = accept;
    send(net::makeMessage(net::kTypeRematch,
                          QJsonObject{{QStringLiteral("accept"), accept}}));
    if (accept) {
        wantRematch_ = true;
        if (state_ != OnlineState::Restarting) {
            setState(OnlineState::Restarting);
            startHeartbeat();
        }
        maybeStartRematch();
    } else {
        wantRematch_ = false;
        setState(OnlineState::GameOver);
        stopHeartbeat();
        stopTimerTick();
        emit rematchDeclined();
    }
    emit logMessage(logPrefix() + QStringLiteral("REMATCH answer accept=%1").arg(accept));
}

// ---------------------------------------------------------------------------
// 心跳 / 通用
// ---------------------------------------------------------------------------

void OnlineSession::startHeartbeat() {
    if (!link_ || !link_->isConnected()) {
        return;
    }
    lastActivityMs_ = QDateTime::currentMSecsSinceEpoch();
    heartbeatTimer_.start();
}

void OnlineSession::stopHeartbeat() {
    heartbeatTimer_.stop();
}

void OnlineSession::onHeartbeatTick() {
    if (!link_ || !link_->isConnected()) {
        stopHeartbeat();
        return;
    }
    send(net::makeMessage(net::kTypePing));
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastActivityMs_ > kHeartbeatTimeoutMs) {
        emit logMessage(logPrefix() + QStringLiteral("heartbeat timeout, peer gone"));
        handlePeerGone();
    }
}

void OnlineSession::onConnectTimeout() {
    if (state_ != OnlineState::Connecting) {
        return;
    }
    if (link_) {
        link_->disconnectPeer();
    }
    setState(OnlineState::Disconnected);
    emit errorOccurred(QStringLiteral("连接超时：无法连接到主机"));
    emit logMessage(logPrefix() + QStringLiteral("connect timeout"));
}

void OnlineSession::resetGame() {
    game_.reset();
    stopTimerTick();
}

void OnlineSession::handlePeerGone() {
    if (state_ == OnlineState::Disconnected || state_ == OnlineState::Error) {
        return;
    }
    stopHeartbeat();
    stopTimerTick();
    connectTimeoutTimer_.stop();
    if (state_ == OnlineState::Connecting) {
        setState(OnlineState::Disconnected);
        emit errorOccurred(QStringLiteral("连接失败：无法连接到主机"));
        return;
    }
    if (state_ == OnlineState::OpponentDisconnected) {
        return;
    }
    setState(OnlineState::OpponentDisconnected);
    emit opponentDisconnected();
    emit logMessage(logPrefix() + QStringLiteral("peer gone"));
}

// ---------------------------------------------------------------------------
// 计时
// ---------------------------------------------------------------------------

void OnlineSession::resetTimers() {
    if (timeLimitMs_ <= 0) {
        blackRemainingMs_ = 0;
        whiteRemainingMs_ = 0;
        activeRemainingMs_ = 0;
        turnStartMs_ = 0;
        stopTimerTick();
        return;
    }
    blackRemainingMs_ = timeLimitMs_;
    whiteRemainingMs_ = timeLimitMs_;
    activeRemainingMs_ = timeLimitMs_;
    turnStartMs_ = 0;
    emit timeUpdated(blackRemainingMs_, whiteRemainingMs_, game_.currentPlayer());
}

void OnlineSession::beginTurn() {
    if (state_ != OnlineState::Playing || game_.status() != GameStatus::InProgress) {
        stopTimerTick();
        return;
    }
    turnStartMs_ = QDateTime::currentMSecsSinceEpoch();
    activeRemainingMs_ = game_.currentPlayer() == Piece::Black ? blackRemainingMs_ : whiteRemainingMs_;
    startTimerTick();
    emit timeUpdated(blackRemainingMs_, whiteRemainingMs_, game_.currentPlayer());
}

void OnlineSession::settleTurn(Piece mover) {
    if (timeLimitMs_ <= 0 || turnStartMs_ <= 0) {
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = now - turnStartMs_;
    if (elapsed < 0) {
        elapsed = 0;
    }
    qint64 remaining = activeRemainingMs_ - elapsed;
    if (remaining < 0) {
        remaining = 0;
    }
    if (mover == Piece::Black) {
        blackRemainingMs_ = remaining;
    } else {
        whiteRemainingMs_ = remaining;
    }
    turnStartMs_ = 0;
}

void OnlineSession::startTimerTick() {
    if (timeLimitMs_ <= 0) {
        return;
    }
    if (!tickTimer_.isActive()) {
        tickTimer_.start();
    }
}

void OnlineSession::stopTimerTick() {
    if (tickTimer_.isActive()) {
        tickTimer_.stop();
    }
}

void OnlineSession::applyTimerState(qint64 blackMs, qint64 whiteMs, Piece current) {
    blackRemainingMs_ = blackMs;
    whiteRemainingMs_ = whiteMs;
    if (state_ == OnlineState::Playing && game_.status() == GameStatus::InProgress) {
        beginTurn();
    } else {
        stopTimerTick();
    }
    emit timeUpdated(blackRemainingMs_, whiteRemainingMs_, current);
}

void OnlineSession::onTimerTick() {
    if (state_ != OnlineState::Playing || game_.status() != GameStatus::InProgress) {
        stopTimerTick();
        return;
    }
    if (timeLimitMs_ <= 0 || turnStartMs_ <= 0) {
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsed = now - turnStartMs_;
    const qint64 remaining = activeRemainingMs_ - elapsed;
    const Piece current = game_.currentPlayer();

    if (isHost() && remaining <= 0) {
        handleTimeout();
        return;
    }

    if (current == Piece::Black) {
        blackRemainingMs_ = remaining < 0 ? 0 : remaining;
    } else {
        whiteRemainingMs_ = remaining < 0 ? 0 : remaining;
    }
    emit timeUpdated(blackRemainingMs_, whiteRemainingMs_, current);
}

void OnlineSession::handleTimeout() {
    if (state_ != OnlineState::Playing) {
        return;
    }
    stopTimerTick();
    const Piece timedOut = game_.currentPlayer();
    const GameStatus status = timedOut == Piece::Black ? GameStatus::WhiteWin : GameStatus::BlackWin;
    game_.forceResult(status);
    setState(OnlineState::GameOver);
    send(net::makeMessage(net::kTypeGameOver,
                          QJsonObject{{QStringLiteral("winner"), winnerName(status)},
                                      {QStringLiteral("reason"), QStringLiteral("TIMEOUT")}}));
    emit gameStatusChanged(status, {}, true);
    stopHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("TIMEOUT -> %1")
                        .arg(status == GameStatus::WhiteWin ? QStringLiteral("WHITE_WIN")
                                                            : QStringLiteral("BLACK_WIN")));
}

void OnlineSession::checkGameOverAfterMove(int row, int col) {
    if (game_.status() != GameStatus::InProgress) {
        setState(OnlineState::GameOver);
        const std::vector<GameMove> line = game_.winningLine(row, col);
        emit gameStatusChanged(game_.status(), line, true);
        stopHeartbeat();
        stopTimerTick();
        send(net::makeMessage(net::kTypeGameOver,
                              QJsonObject{{QStringLiteral("winner"), winnerName(game_.status())}}));
        emit logMessage(logPrefix() + QStringLiteral("GAME_OVER winner=%1")
                            .arg(winnerName(game_.status())));
    } else {
        beginTurn();
    }
}

// ---------------------------------------------------------------------------
// 聊天
// ---------------------------------------------------------------------------

void OnlineSession::sendChat(const QString& text) {
    if (!link_ || !link_->isConnected()) {
        emit chatSendFailed(QStringLiteral("未连接到主机"));
        return;
    }
    if (text.trimmed().isEmpty()) {
        emit chatSendFailed(QStringLiteral("消息不能为空"));
        return;
    }
    if (text.size() > net::kMaxChatLength) {
        emit chatSendFailed(QStringLiteral("消息过长（最多 %1 字）").arg(net::kMaxChatLength));
        return;
    }
    send(net::makeMessage(net::kTypeChat,
                          QJsonObject{{QStringLiteral("text"), text}}));
    const Piece sender = myColor_;
    emit chatMessageReceived(sender, text, QDateTime::currentMSecsSinceEpoch());
    emit logMessage(logPrefix() + QStringLiteral("CHAT sent"));
}

void OnlineSession::handleChat(const QJsonObject& data) {
    const QString text = data.value(QStringLiteral("text")).toString();
    if (text.trimmed().isEmpty()) {
        emit logMessage(logPrefix() + QStringLiteral("CHAT ignored (empty)"));
        return;
    }
    if (text.size() > net::kMaxChatLength) {
        emit logMessage(logPrefix() + QStringLiteral("CHAT ignored (too long)"));
        send(net::makeMessage(net::kTypeReject,
                              QJsonObject{{QStringLiteral("reason"), QStringLiteral("CHAT_TOO_LONG")}}));
        return;
    }
    // 客户端已在本地回显，这里只记录到主机侧，不再转发回去
    const Piece sender = myColor_ == Piece::Black ? Piece::White : Piece::Black;
    emit chatMessageReceived(sender, text, QDateTime::currentMSecsSinceEpoch());
    emit logMessage(logPrefix() + QStringLiteral("CHAT relayed"));
}

void OnlineSession::applyChat(const QJsonObject& data) {
    const QString text = data.value(QStringLiteral("text")).toString();
    if (text.trimmed().isEmpty()) {
        return;
    }
    if (text.size() > net::kMaxChatLength) {
        return;
    }
    const Piece sender = myColor_ == Piece::Black ? Piece::White : Piece::Black;
    emit chatMessageReceived(sender, text, QDateTime::currentMSecsSinceEpoch());
}

} // namespace Gomoku
