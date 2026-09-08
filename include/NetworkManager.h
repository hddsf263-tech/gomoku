#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <functional>

namespace Gomoku {

class NetworkManager {
public:
    enum class Role { None, Host, Client };

    using VoidCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const QString&)>;
    using MoveCallback = std::function<void(int, int)>;
    using ColorCallback = std::function<void(int)>;

    NetworkManager();
    ~NetworkManager();

    bool listen(quint16 port);
    void connectToHost(const QString& host, quint16 port);
    void disconnectPeer();

    bool isConnected() const { return connected_; }
    Role role() const { return role_; }

    void sendHello(int color);
    void sendMove(int row, int col);
    void sendReset();

    void onConnected(VoidCallback cb) { onConnected_ = std::move(cb); }
    void onDisconnected(VoidCallback cb) { onDisconnected_ = std::move(cb); }
    void onError(ErrorCallback cb) { onError_ = std::move(cb); }
    void onMove(MoveCallback cb) { onMove_ = std::move(cb); }
    void onHello(ColorCallback cb) { onHello_ = std::move(cb); }
    void onReset(VoidCallback cb) { onReset_ = std::move(cb); }

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
};

} // namespace Gomoku
