#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

// 新联机系统协议层：消息类型常量 + JSON 消息构造/帧化/解析工具。
// 仅负责"表示"，不依赖任何棋局核心，便于单独测试。
namespace Gomoku::net {

// ---- 消息类型常量 --------------------------------------------------------
inline constexpr const char* kTypeJoin      = "JOIN";         // 客户端 -> 主机：请求加入
inline constexpr const char* kTypeWelcome   = "WELCOME";      // 主机 -> 客户端：分配颜色
inline constexpr const char* kTypeState     = "STATE";        // 主机 -> 客户端：整局棋盘状态同步
inline constexpr const char* kTypeMove      = "MOVE";         // 主机 -> 客户端：广播已判定合法的落子
inline constexpr const char* kTypeReqMove   = "REQ_MOVE";     // 客户端 -> 主机：请求落子
inline constexpr const char* kTypeReject    = "REJECT";       // 主机 -> 客户端：拒绝落子（附原因）
inline constexpr const char* kTypeGameOver  = "GAME_OVER";    // 主机 -> 客户端：终局判定
inline constexpr const char* kTypeNewGame   = "NEW_GAME";     // 主机 -> 客户端：重赛开局
inline constexpr const char* kTypeRematch   = "REMATCH";      // 双方：重赛请求/应答 {accept}
inline constexpr const char* kTypeBye       = "BYE";          // 双方：主动离开
inline constexpr const char* kTypeResign    = "RESIGN";       // 双方：一方投降 {resigner}
inline constexpr const char* kTypeChat      = "CHAT_MESSAGE"; // 双方：聊天 {sender,message,ts}
inline constexpr const char* kTypePing      = "PING";         // 心跳
inline constexpr const char* kTypePong      = "PONG";         // 心跳应答

// ---- 默认端口 ------------------------------------------------------------
inline constexpr quint16 kDefaultPort = 12345;

// ---- 聊天限制 ------------------------------------------------------------
inline constexpr int kMaxChatLength = 200;   // 单条聊天消息最大字符数
inline constexpr int kDefaultTimeMinutes = 10; // 默认对局时长（分钟）

// ---- 消息构造 ------------------------------------------------------------
// 全消息统一带 type + version（协议版本），便于扩展。
inline QJsonObject makeMessage(const char* type) {
    QJsonObject obj;
    obj["type"] = QString::fromLatin1(type);
    obj["version"] = 1;
    return obj;
}

// 构造并合并业务数据字段。
inline QJsonObject makeMessage(const char* type, const QJsonObject& data) {
    QJsonObject obj = makeMessage(type);
    for (auto it = data.begin(); it != data.end(); ++it) {
        obj[it.key()] = it.value();
    }
    return obj;
}

// 读取消息类型。
inline QString typeOf(const QJsonObject& obj) {
    return obj.value(QStringLiteral("type")).toString();
}

// ---- 帧化 / 解析 ---------------------------------------------------------
// 序列化：紧凑 JSON + '\n' 行分隔（清晰消息边界，天然解决 TCP 半包/粘包/拆包）。
inline QByteArray frame(const QJsonObject& obj) {
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    data.append('\n');
    return data;
}

// 解析一行 JSON。成功且为 object 时返回 true。
inline bool parse(const QByteArray& line, QJsonObject& out) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }
    out = doc.object();
    return true;
}

} // namespace Gomoku::net
