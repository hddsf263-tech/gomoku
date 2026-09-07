#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <functional>

namespace Gomoku {

/// @brief 双人联机网络管理：主机(监听)/客户端(连接)，使用 Qt TCP + JSON 消息。
/// 以 std::function 回调通知上层，非 QObject 类，避免额外的 moc 步骤。
class NetworkManager {
public:
    enum class Role { None, Host, Client };

    using VoidCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const QString&)>;
    using MoveCallback = std::function<void(int, int)>;
    using ColorCallback = std::function<void(int)>;  ///< 0=黑, 1=白
    using BoolCallback = std::function<void(bool)>;  ///< 接受/拒绝

    NetworkManager();
    ~NetworkManager();

    /// @brief 作为主机开始监听
    bool listen(quint16 port);
    /// @brief 作为客户端连接主机
    void connectToHost(const QString& host, quint16 port);
    /// @brief 断开当前连接
    void disconnectPeer();

    bool isConnected() const { return connected_; }
    Role role() const { return role_; }

    void sendHello(int color);
    void sendMove(int row, int col);
    void sendReset();
    void sendSurrender();
    void sendDrawOffer();
    void sendDrawResponse(bool accept);

    void onConnected(VoidCallback cb) { onConnected_ = std::move(cb); }
    void onDisconnected(VoidCallback cb) { onDisconnected_ = std::move(cb); }
    void onError(ErrorCallback cb) { onError_ = std::move(cb); }
    void onMove(MoveCallback cb) { onMove_ = std::move(cb); }
    void onHello(ColorCallback cb) { onHello_ = std::move(cb); }
    void onReset(VoidCallback cb) { onReset_ = std::move(cb); }
    void onSurrender(VoidCallback cb) { onSurrender_ = std::move(cb); }
    void onDrawOffer(VoidCallback cb) { onDrawOffer_ = std::move(cb); }
    void onDrawResponse(BoolCallback cb) { onDrawResponse_ = std::move(cb); }

private:
    void attachSocket(QTcpSocket* socket);
    void readAvailable();
    void dispatch(const QByteArray& json);
    void sendRaw(const QByteArray& json);

    QTcpServer* server_;
    QTcpSocket* socket_;
    QByteArray buffer_;
    Role role_;
    bool connected_;

    VoidCallback onConnected_;
    VoidCallback onDisconnected_;
    ErrorCallback onError_;
    MoveCallback onMove_;
    ColorCallback onHello_;
    VoidCallback onReset_;
    VoidCallback onSurrender_;
    VoidCallback onDrawOffer_;
    BoolCallback onDrawResponse_;
};

} // namespace Gomoku

#endif // NETWORK_MANAGER_H
