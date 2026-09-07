#include "NetworkManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>

namespace Gomoku {

NetworkManager::NetworkManager()
    : server_(new QTcpServer)
    , socket_(nullptr)
    , role_(Role::None)
    , connected_(false)
{
    QObject::connect(server_, &QTcpServer::newConnection, [this]() {
        QTcpSocket* client = server_->nextPendingConnection();
        if (socket_) {
            socket_->disconnectFromHost();
            socket_->deleteLater();
            socket_ = nullptr;
        }
        attachSocket(client);
    });
}

NetworkManager::~NetworkManager() {
    delete socket_;
    delete server_;
}

bool NetworkManager::listen(quint16 port) {
    role_ = Role::Host;
    return server_->listen(QHostAddress::Any, port);
}

void NetworkManager::connectToHost(const QString& host, quint16 port) {
    role_ = Role::Client;
    auto* s = new QTcpSocket;
    attachSocket(s);
    QObject::connect(s, &QTcpSocket::connected, [this]() {
        connected_ = true;
        if (onConnected_) {
            onConnected_();
        }
    });
    s->connectToHost(host, port);
}

void NetworkManager::disconnectPeer() {
    if (socket_) {
        socket_->disconnectFromHost();
        socket_->deleteLater();
        socket_ = nullptr;
    }
    buffer_.clear();
    connected_ = false;
}

void NetworkManager::attachSocket(QTcpSocket* socket) {
    socket_ = socket;
    buffer_.clear();

    QObject::connect(socket_, &QTcpSocket::readyRead, [this]() {
        readAvailable();
    });
    QObject::connect(socket_, &QTcpSocket::disconnected, [this]() {
        connected_ = false;
        if (onDisconnected_) {
            onDisconnected_();
        }
    });
    QObject::connect(socket_, &QTcpSocket::errorOccurred,
                     [this](QAbstractSocket::SocketError) {
        if (onError_) {
            onError_(socket_ ? socket_->errorString() : QString());
        }
    });

    if (role_ == Role::Host) {
        connected_ = true;
        if (onConnected_) {
            onConnected_();
        }
    }
}

void NetworkManager::sendRaw(const QByteArray& json) {
    if (socket_ && connected_) {
        socket_->write(json);
        socket_->write("\n", 1);
    }
}

void NetworkManager::sendHello(int color) {
    QJsonObject obj;
    obj["type"] = "HELLO";
    obj["color"] = color;
    sendRaw(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void NetworkManager::sendMove(int row, int col) {
    QJsonObject obj;
    obj["type"] = "MOVE";
    obj["row"] = row;
    obj["col"] = col;
    sendRaw(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void NetworkManager::sendReset() {
    QJsonObject obj;
    obj["type"] = "RESET";
    sendRaw(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void NetworkManager::readAvailable() {
    if (!socket_) {
        return;
    }
    buffer_.append(socket_->readAll());
    int idx;
    while ((idx = buffer_.indexOf('\n')) != -1) {
        QByteArray line = buffer_.left(idx);
        buffer_.remove(0, idx + 1);
        if (!line.trimmed().isEmpty()) {
            dispatch(line);
        }
    }
}

void NetworkManager::dispatch(const QByteArray& json) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (onError_) {
            onError_("收到无法解析的消息");
        }
        return;
    }

    const QJsonObject obj = doc.object();
    const QString type = obj["type"].toString();
    if (type == "HELLO" && onHello_) {
        onHello_(obj["color"].toInt());
    } else if (type == "MOVE" && onMove_) {
        onMove_(obj["row"].toInt(), obj["col"].toInt());
    } else if (type == "RESET" && onReset_) {
        onReset_();
    }
}

} // namespace Gomoku
