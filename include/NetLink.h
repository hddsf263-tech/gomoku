#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

class QTcpServer;
class QTcpSocket;

// 新联机系统传输层：负责 TCP 监听/连接、换行分隔 JSON 帧化、事件信号。
// 纯传输职责，不接触棋局核心。
namespace Gomoku::net {

class NetLink : public QObject {
    Q_OBJECT
public:
    enum class Role { None, Host, Client };

    explicit NetLink(QObject* parent = nullptr);
    ~NetLink() override;

    // Host：开始监听端口。成功返回 true。
    bool listen(quint16 port);
    // Client：连接指定主机。
    void connectToHost(const QString& host, quint16 port);
    // 主动断开（用户发起）。
    void disconnectPeer();
    // 发送一条 JSON 消息（若当前连接可用）。
    void send(const QJsonObject& obj);

    Role role() const { return role_; }
    bool isHosting() const;
    bool isConnected() const { return connected_; }
    bool isActive() const { return isHosting() || isConnected(); }
    // Host 实际监听的端口（listen(0) 时用于查询分配的端口）。
    quint16 port() const;

signals:
    void connected();            // TCP 握手完成（Client）或对端加入（Host）
    void disconnected();         // 连接断开（无论何种原因）
    void errorOccurred(const QString& message);
    void messageReceived(const QJsonObject& obj);
    void peerConnected();        // 仅 Host：一个客户端已加入
    void peerDisconnected();     // 仅 Host：已加入的客户端断开

private:
    void attachSocket(QTcpSocket* socket);
    void readAvailable();
    void handleDisconnect();

    Role role_ = Role::None;
    QTcpServer* server_ = nullptr;
    QTcpSocket* socket_ = nullptr;
    QByteArray buffer_;
    bool connected_ = false;
};

} // namespace Gomoku::net
