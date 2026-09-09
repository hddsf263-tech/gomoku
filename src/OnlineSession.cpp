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
}

OnlineSession::~OnlineSession() {
    leaving_ = true;
    if (link_) {
        link_->disconnectPeer();
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
    connectTimeoutTimer_.stop();
    if (link_) {
        link_->disconnectPeer();
    }
    myColor_ = Piece::Black;
    port_ = port;
    wantRematch_ = false;
    oppRematch_ = false;
    game_.reset();

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
    connectTimeoutTimer_.stop();
    if (link_) {
        link_->disconnectPeer();
    }
    myColor_ = Piece::White; // 最终以 WELCOME 为准
    hostAddress_ = host;
    port_ = port;
    wantRematch_ = false;
    oppRematch_ = false;
    game_.reset();

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
    connectTimeoutTimer_.stop();
    if (link_) {
        send(net::makeMessage(net::kTypeBye));
        link_->disconnectPeer();
    }
    game_.reset();
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
        // 主机已在 peer joined 时完成分配；JOIN 仅用于确认。
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
            clientApplyNewGame();
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
    return true;
}

void OnlineSession::hostCommitMove(int row, int col, Piece player) {
    const Piece placed = game_.currentPlayer();
    if (!game_.makeMove(row, col)) {
        emit errorOccurred(QStringLiteral("主机校验与落子不一致"));
        return;
    }
    send(net::makeMessage(net::kTypeMove,
                          QJsonObject{{QStringLiteral("x"), col},
                                      {QStringLiteral("y"), row},
                                      {QStringLiteral("piece"), static_cast<int>(placed)}}));
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
    send(net::makeMessage(net::kTypeState, data));
}

void OnlineSession::hostStartNewGame() {
    game_.reset();
    setState(OnlineState::Playing);
    send(net::makeMessage(net::kTypeNewGame));
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
    // 等待 STATE 到来后进入对局。
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
        }
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
    emit moveCommitted(row, col, placed, game_.status());
    emit logMessage(logPrefix() + QStringLiteral("MOVE applied x=%1 y=%2").arg(col).arg(row));
    if (game_.status() != GameStatus::InProgress) {
        setState(OnlineState::GameOver);
        const std::vector<GameMove> line = game_.winningLine(row, col);
        emit gameStatusChanged(game_.status(), line, true);
        stopHeartbeat();
        emit logMessage(logPrefix() + QStringLiteral("GAME_OVER winner=%1")
                            .arg(winnerName(game_.status())));
    }
}

void OnlineSession::clientApplyGameOver(const QJsonObject& data) {
    if (state_ == OnlineState::GameOver) {
        return; // 已通过 MOVE 应用过，幂等
    }
    const QString winner = data.value(QStringLiteral("winner")).toString();
    GameStatus status = GameStatus::Draw;
    if (winner == QStringLiteral("BLACK")) {
        status = GameStatus::BlackWin;
    } else if (winner == QStringLiteral("WHITE")) {
        status = GameStatus::WhiteWin;
    }
    setState(OnlineState::GameOver);
    const auto last = game_.lastMove();
    std::vector<GameMove> line;
    if (last.has_value() && status != GameStatus::Draw) {
        line = game_.winningLine(last->row, last->col);
    }
    emit gameStatusChanged(status, line, true);
    stopHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("GAME_OVER (explicit) winner=%1").arg(winner));
}

void OnlineSession::clientApplyNewGame() {
    resetGame();
    setState(OnlineState::Playing);
    emit sessionStarted();
    emit rematchAccepted();
    startHeartbeat();
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
            // 主机本端非法输入，静默忽略（不会向自己发 REJECT）。
            emit logMessage(logPrefix() + QStringLiteral("local host move rejected: %1").arg(reason));
            return false;
        }
        hostCommitMove(row, col, myColor_);
        return true;
    }
    // 客户端：发送 REQ_MOVE，等待主机回 MOVE 后落地。
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
    const Piece resigner = myColor_;
    const GameStatus status = resigner == Piece::Black ? GameStatus::WhiteWin : GameStatus::BlackWin;
    game_.forceResult(status);
    setState(OnlineState::GameOver);
    emit resigned(resigner, status);
    emit gameStatusChanged(status, {}, true);
    send(net::makeMessage(net::kTypeResign, QJsonObject{{QStringLiteral("resigner"), static_cast<int>(resigner)}}));
    stopHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("RESIGN by %1 -> %2")
                        .arg(resigner == Piece::Black ? "BLACK" : "WHITE")
                        .arg(status == GameStatus::WhiteWin ? "WHITE_WIN" : "BLACK_WIN"));
}

void OnlineSession::answerRematch(bool accept) {
    if (state_ != OnlineState::GameOver && state_ != OnlineState::Restarting) {
        return;
    }
    wantRematch_ = accept;
    if (accept) {
        oppRematch_ = true;
        setState(OnlineState::Restarting);
        startHeartbeat();
        send(net::makeMessage(net::kTypeRematch, QJsonObject{{QStringLiteral("accept"), true}}));
        emit logMessage(logPrefix() + QStringLiteral("REMATCH accepted"));
        maybeStartRematch();
    } else {
        oppRematch_ = false;
        wantRematch_ = false;
        setState(OnlineState::GameOver);
        stopHeartbeat();
        send(net::makeMessage(net::kTypeRematch, QJsonObject{{QStringLiteral("accept"), false}}));
        emit logMessage(logPrefix() + QStringLiteral("REMATCH declined"));
    }
}

// ---------------------------------------------------------------------------
// 心跳 / 通用
// ---------------------------------------------------------------------------

void OnlineSession::startHeartbeat() {
    lastActivityMs_ = QDateTime::currentMSecsSinceEpoch();
    if (!heartbeatTimer_.isActive()) {
        heartbeatTimer_.start();
    }
}

void OnlineSession::stopHeartbeat() {
    heartbeatTimer_.stop();
}

void OnlineSession::onHeartbeatTick() {
    if (!isConnected()) {
        stopHeartbeat();
        return;
    }
    if (state_ == OnlineState::Connecting) {
        return;
    }
    send(net::makeMessage(net::kTypePing));
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastActivityMs_ > 0 && (now - lastActivityMs_) >= kHeartbeatTimeoutMs) {
        emit logMessage(logPrefix() + QStringLiteral("heartbeat timeout"));
        handlePeerGone();
    }
}

void OnlineSession::onConnectTimeout() {
    if (state_ == OnlineState::Connecting && !isConnected()) {
        emit errorOccurred(QStringLiteral("连接超时，未能连接到主机"));
        emit logMessage(logPrefix() + QStringLiteral("connect timeout"));
        if (link_) {
            link_->disconnectPeer();
        }
        setState(OnlineState::Disconnected);
    }
}

void OnlineSession::resetGame() {
    game_.reset();
}

void OnlineSession::handlePeerGone() {
    stopHeartbeat();
    connectTimeoutTimer_.stop();
    if (state_ == OnlineState::Disconnected) {
        return;
    }
    setState(OnlineState::OpponentDisconnected);
    emit opponentDisconnected();
    emit logMessage(logPrefix() + QStringLiteral("opponent disconnected"));
}

void OnlineSession::checkGameOverAfterMove(int row, int col) {
    if (game_.status() == GameStatus::InProgress) {
        return;
    }
    setState(OnlineState::GameOver);
    const std::vector<GameMove> line = game_.winningLine(row, col);
    emit gameStatusChanged(game_.status(), line, true);
    send(net::makeMessage(net::kTypeGameOver,
                          QJsonObject{{QStringLiteral("winner"), winnerName(game_.status())}}));
    stopHeartbeat();
    emit logMessage(logPrefix() + QStringLiteral("GAME_OVER winner=%1")
                        .arg(winnerName(game_.status())));
}

} // namespace Gomoku
