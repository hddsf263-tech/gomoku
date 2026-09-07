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

    // ---- UI 交互 ----
    bool localMove(int row, int col);
    void requestRematch();
    void answerRematch(bool accept);

signals:
    void stateChanged(OnlineState state);
    void logMessage(const QString& text);
    void colorAssigned(Piece color);
    void sessionStarted();     // 双方就绪、棋盘已清空、进入对局
    void moveCommitted(int row, int col, Piece piece, GameStatus status);
    void moveRejected(const QString& reason);
    void gameStatusChanged(GameStatus status, const std::vector<GameMove>& line, bool online);
    void rematchRequested();
    void rematchAccepted();
    void rematchDeclined();
    void opponentDisconnected();
    void errorOccurred(const QString& text);

private slots:
    void onLinkConnected();
    void onLinkDisconnected();
    void onLinkError(const QString& text);
    void onMessage(const QJsonObject& obj);
    void onHeartbeatTick();
    void onConnectTimeout();

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
    void clientApplyNewGame();

    // 重赛
    void handleRematch(const QJsonObject& data);

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
    qint64 lastActivityMs_ = 0;

    bool wantRematch_ = false;
    bool oppRematch_ = false;
    bool leaving_ = false;
};

} // namespace Gomoku