#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>
#include <vector>

#include "GomokuCore.h"

namespace Gomoku::net {
class NetLink;
}

namespace Gomoku {

// 联机会话状态机。
enum class OnlineState {
    Disconnected,         // 未连接
    Connecting,           // 客户端正在连接
    WaitingOpponent,      // 主机已监听，等待对手加入
    Playing,              // 对局中
    GameOver,             // 对局结束
    Restarting,           // 重赛协商中（保留，可用于后续细分）
    OpponentDisconnected, // 对手断开
    Error                 // 错误
};

QString onlineStateName(OnlineState state);

/// 联机会话层：协调对局、Host 权威裁判、状态同步、重赛、断线。
///
/// 只依赖 GomokuCore（棋局核心）、NetLink（TCP 传输）、NetProtocol（协议表示）。
/// 通过引用注入 GameEngine 作为棋局核心：Host 权威、Client 镜像。
/// 事件驱动，不使用任何阻塞循环，不调用 while(true)。
class OnlineSession : public QObject {
    Q_OBJECT

public:
    explicit OnlineSession(GameEngine& game, QObject* parent = nullptr);
    ~OnlineSession() override;

    // ---- 角色入口 ----
    bool startHost(quint16 port);
    void connectToHost(const QString& host, quint16 port);
    void leaveSession();

    // ---- 状态查询 ----
    bool isActive() const { return state_ != OnlineState::Disconnected; }
    bool isHost() const;
    bool isConnected() const;
    OnlineState state() const { return state_; }
    Piece myColor() const { return myColor_; }
    quint16 port() const { return port_; }
    QString hostAddress() const { return hostAddress_; }

    // ---- 计时设置 ----
    // 设置对局时长（毫秒）；0 表示不限时。应在 startHost / connectToHost 之前调用。
    void setTimeLimitMs(qint64 ms);
    qint64 timeLimitMs() const { return timeLimitMs_; }

    // ---- UI 交互 ----
    bool localMove(int row, int col);
    void requestRematch();
    void resign();
    void answerRematch(bool accept);
    void sendChat(const QString& text);

signals:
    void stateChanged(OnlineState state);
    void logMessage(const QString& text);
    void colorAssigned(Piece color);
    void sessionStarted();     // 双方就绪、棋盘已清空、进入对局
    void moveCommitted(int row, int col, Piece piece, GameStatus status);
    void moveRejected(const QString& reason);
    void resigned(Piece resigner, GameStatus status);
    void gameStatusChanged(GameStatus status, const std::vector<GameMove>& line, bool online);
    void rematchRequested();
    void rematchAccepted();
    void rematchDeclined();
    void opponentDisconnected();
    void errorOccurred(const QString& text);

    // 计时器：黑/白剩余毫秒、当前玩家。由 Host/Client 的本地计时器周期发出。
    void timeUpdated(qint64 blackRemainingMs, qint64 whiteRemainingMs, Piece currentPlayer);
    // 聊天：发送者（Piece）、消息文本、时间戳（ms）。双方本地都收到同一条记录。
    void chatMessageReceived(Piece sender, const QString& text, qint64 timestampMs);
    // 聊天被拒绝（如未连接、过长）。
    void chatSendFailed(const QString& reason);

private slots:
    void onLinkConnected();
    void onLinkDisconnected();
    void onLinkError(const QString& text);
    void onMessage(const QJsonObject& obj);
    void onHeartbeatTick();
    void onConnectTimeout();
    void onTimerTick();

private:
    void setState(OnlineState s);
    QString logPrefix() const { return QStringLiteral("[NETWORK] "); }

    void send(const QJsonObject& obj);

    // Host 权威逻辑
    void hostOnPeerJoined();
    bool hostValidateMove(int row, int col, Piece player, QString& reason) const;
    void hostCommitMove(int row, int col, Piece player);
    void hostBroadcastState();
    void hostStartNewGame();
    void maybeStartRematch();

    // Client 逻辑
    void clientSendJoin();
    void clientApplyWelcome(const QJsonObject& data);
    void clientApplyState(const QJsonObject& data);
    void clientApplyMove(const QJsonObject& data);
    void clientApplyGameOver(const QJsonObject& data);
    void clientApplyNewGame(const QJsonObject& data);

    // 重赛
    void handleRematch(const QJsonObject& data);
    void handleResign(const QJsonObject& data, const Piece resigner);

    // 计时
    void resetTimers();
    void beginTurn();
    void settleTurn(Piece mover);
    void handleTimeout();
    void startTimerTick();
    void stopTimerTick();
    void applyTimerState(qint64 blackMs, qint64 whiteMs, Piece current);

    // 聊天
    void handleChat(const QJsonObject& data);   // Host：校验并转发
    void applyChat(const QJsonObject& data);    // Client：接收并上抛

    // 心跳 / 通用
    void startHeartbeat();
    void stopHeartbeat();
    void resetGame();
    void handlePeerGone();
    void checkGameOverAfterMove(int row, int col);

    GameEngine& game_;

    std::unique_ptr<net::NetLink> link_;

    OnlineState state_ = OnlineState::Disconnected;
    Piece myColor_ = Piece::Black;
    quint16 port_ = 0;
    QString hostAddress_;

    QTimer heartbeatTimer_;
    QTimer connectTimeoutTimer_;
    QTimer tickTimer_;
    qint64 lastActivityMs_ = 0;

    bool wantRematch_ = false;
    bool oppRematch_ = false;
    bool leaving_ = false;

    // 计时器状态（Host 权威；Client 用于显示）
    qint64 timeLimitMs_ = 600000; // 默认 10 分钟
    qint64 blackRemainingMs_ = 0;
    qint64 whiteRemainingMs_ = 0;
    qint64 activeRemainingMs_ = 0;  // 当前回合玩家在回合开始时剩余
    qint64 turnStartMs_ = 0;         // 当前回合开始时间戳（epoch ms）
};

} // namespace Gomoku
