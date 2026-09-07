#include "NetLink.h"
#include "NetProtocol.h"

#include <QHostAddress>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

namespace Gomoku::net {

NetLink::NetLink(QObject* parent)
    : QObject(parent)
    , server_(new QTcpServer(this))
{
    connect(server_, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket* client = server_->nextPendingConnection();
        if (!client) {
            return;
        }
        // 只允许一个对端：若已有连接则先断开旧的。
        if (socket_) {
            socket_->disconnectFromHost();
            socket_->deleteLater();
            socket_ = nullptr;
        }
        attachSocket(client);
        connected_ = true;
        emit peerConnected();
        emit connected();
    });
}

NetLink::~NetLink() {
    disconnectPeer();
}

bool NetLink::listen(quint16 port) {
    role_ = Role::Host;
    connected_ = false;
    return server_->listen(QHostAddress::Any, port);
}

void NetLink::connectToHost(const QString& host, quint16 port) {
    role_ = Role::Client;
    auto* socket = new QTcpSocket(this);
    attachSocket(socket);
    connect(socket, &QTcpSocket::connected, this, [this]() {
        connected_ = true;
        emit connected();
    });
    socket->connectToHost(host, port);
}

void NetLink::disconnectPeer() {
    if (socket_) {
        socket_->blockSignals(true);
        socket_->abort();
        socket_->deleteLater();
        socket_ = nullptr;
    }
    buffer_.clear();
    connected_ = false;
    if (server_->isListening()) {
        server_->close();
    }
    role_ = Role::None;
}

void NetLink::send(const QJsonObject& obj) {
    if (socket_ && connected_) {
        socket_->write(frame(obj));
    }
}

bool NetLink::isHosting() const {
    return server_ && server_->isListening();
}

quint16 NetLink::port() const {
    return server_ ? server_->serverPort() : 0;
}

void NetLink::attachSocket(QTcpSocket* socket) {
    socket_ = socket;
    socket_->setParent(this);
    buffer_.clear();

    connect(socket_, &QTcpSocket::readyRead, this, &NetLink::readAvailable);
    connect(socket_, &QTcpSocket::disconnected, this, &NetLink::handleDisconnect);
    connect(socket_, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
        if (socket_) {
            emit errorOccurred(socket_->errorString());
        }
    });
}

void NetLink::readAvailable() {
    if (!socket_) {
        return;
    }
    buffer_.append(socket_->readAll());
    int idx;
    while ((idx = buffer_.indexOf('\n')) != -1) {
        QByteArray line = buffer_.left(idx);
        buffer_.remove(0, idx + 1);
        if (line.trimmed().isEmpty()) {
            continue;
        }
        QJsonObject obj;
        if (parse(line, obj)) {
            emit messageReceived(obj);
        } else {
            emit errorOccurred(QObject::tr("收到无法解析的消息"));
        }
    }
}

void NetLink::handleDisconnect() {
    const bool wasConnected = connected_;
    connected_ = false;
    if (socket_) {
        socket_->deleteLater();
        socket_ = nullptr;
    }
    emit disconnected();
    if (role_ == Role::Host && wasConnected) {
        emit peerDisconnected();
    }
}

} // namespace Gomoku::net
