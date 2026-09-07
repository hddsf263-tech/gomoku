#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QHostAddress>

#include <cstdio>
#include <functional>

#include "GomokuCore.h"
#include "NetLink.h"
#include "NetProtocol.h"
#include "OnlineSession.h"

using namespace Gomoku;

static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond, msg) do { \
    g_checks++; \
    if (!(cond)) { g_fails++; std::printf("FAIL: %s\n", msg); } \
    else { std::printf("PASS: %s\n", msg); } \
    std::fflush(stdout); \
} while (0)

static bool waitUntil(const std::function<bool()>& cond, int timeoutMs) {
    QEventLoop loop;
    bool done = false;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, [&]() {
        if (cond()) { done = true; loop.quit(); }
    });
    poll.start(20);
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&]() { loop.quit(); });
    timer.start(timeoutMs);
    loop.exec();
    return done;
}

// 测试 A：协议帧化 / 解析
static void testProtocol() {
    std::printf("--- Test A: NetProtocol frame/parse ---\n");
    QJsonObject msg = net::makeMessage(net::kTypeMove,
        QJsonObject{{QStringLiteral("x"), 3}, {QStringLiteral("y"), 4}});
    QByteArray f = net::frame(msg);
    QJsonObject parsed;
    CHECK(net::parse(f, parsed), "frame/parse roundtrip");
    CHECK(net::typeOf(parsed) == QString::fromLatin1(net::kTypeMove), "type preserved");
    CHECK(parsed.value(QStringLiteral("x")).toInt() == 3, "x preserved");
    CHECK(parsed.value(QStringLiteral("y")).toInt() == 4, "y preserved");
    CHECK(msg.value(QStringLiteral("version")).toInt() == 1, "version present");
    CHECK(!net::parse(QByteArray("not json"), parsed), "invalid json rejected");
}

// 测试 B：主机 + 客户端连接并同步落子
static void testConnectAndSync() {
    std::printf("--- Test B: connect + sync moves ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);

    CHECK(host.startHost(0), "host startHost(0)");
    const quint16 port = host.port();
    CHECK(port != 0, "host assigned port");

    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    bool ok = waitUntil([&]() {
        return host.state() == OnlineState::Playing && client.state() == OnlineState::Playing;
    }, 6000);
    CHECK(ok, "both reach Playing");
    CHECK(host.isHost(), "host.isHost()");
    CHECK(host.myColor() == Piece::Black, "host myColor=Black");
    CHECK(client.myColor() == Piece::White, "client myColor=White");

    // Host 黑方先手
    CHECK(host.localMove(7, 7), "host localMove (7,7)");
    bool ok2 = waitUntil([&]() { return clientGame.moveCount() == 1; }, 4000);
    CHECK(ok2, "client received move 1");
    CHECK(hostGame.pieceAt(7, 7) == Piece::Black, "host board black at (7,7)");
    CHECK(clientGame.pieceAt(7, 7) == Piece::Black, "client board black at (7,7)");

    // Client 白方
    CHECK(client.localMove(8, 8), "client localMove (8,8)");
    bool ok3 = waitUntil([&]() { return hostGame.moveCount() == 2; }, 4000);
    CHECK(ok3, "host received move 2");
    CHECK(hostGame.pieceAt(8, 8) == Piece::White, "host board white at (8,8)");
    CHECK(clientGame.pieceAt(8, 8) == Piece::White, "client board white at (8,8)");

    host.leaveSession();
    client.leaveSession();
}

// 测试 C：非法落子被拒绝
static void testIllegalMoveRejected() {
    std::printf("--- Test C: illegal move rejected ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    bool rejected = false;
    QObject::connect(&client, &OnlineSession::moveRejected, [&](const QString&) { rejected = true; });

    CHECK(host.startHost(0), "host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "both reach Playing");

    // 黑方在 (7,7) 落子
    CHECK(host.localMove(7, 7), "host localMove (7,7)");
    CHECK(waitUntil([&]() { return clientGame.moveCount() == 1; }, 4000), "client move 1");

    // 白方在已被占用的 (7,7) 落子（客户端本地不校验，主机应拒绝）
    CHECK(client.localMove(7, 7), "client sends occupied move request");
    CHECK(waitUntil([&]() { return rejected; }, 4000), "client received REJECT");
    CHECK(hostGame.moveCount() == 1, "host did not commit illegal move");

    host.leaveSession();
    client.leaveSession();
}

// 测试 D：终局广播 + 重赛
static void testWinAndRematch() {
    std::printf("--- Test D: win broadcast + rematch ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);

    CHECK(host.startHost(0), "host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "both reach Playing");

    // 让黑方（主机）在中间行连五：黑方 (7,3)(7,4)(7,5)(7,6)(7,7)，白方(0,0)(0,1)(0,2)(0,3)
    const int blackMoves[5][2] = {{7,3},{7,4},{7,5},{7,6},{7,7}};
    const int whiteMoves[4][2] = {{0,0},{0,1},{0,2},{0,3}};
    for (int i = 0; i < 4; i++) {
        CHECK(host.localMove(blackMoves[i][0], blackMoves[i][1]), "black move");
        CHECK(waitUntil([&]() { return clientGame.moveCount() == hostGame.moveCount(); }, 4000),
              "client synced black");
        CHECK(client.localMove(whiteMoves[i][0], whiteMoves[i][1]), "white move");
        CHECK(waitUntil([&]() { return hostGame.moveCount() == (2 * i + 2); }, 4000),
              "host synced white");
    }
    // 第 9 手：黑方制胜
    CHECK(host.localMove(blackMoves[4][0], blackMoves[4][1]), "black winning move");
    CHECK(waitUntil([&]() { return clientGame.moveCount() == 9; }, 4000), "client synced winning move");
    CHECK(waitUntil([&]() {
        return host.state() == OnlineState::GameOver && client.state() == OnlineState::GameOver;
    }, 4000), "both reach GameOver");
    CHECK(hostGame.status() == GameStatus::BlackWin, "host status BlackWin");
    CHECK(clientGame.status() == GameStatus::BlackWin, "client status BlackWin");

    // 重赛
    bool requested = false;
    QObject::connect(&client, &OnlineSession::rematchRequested, [&]() {
        requested = true;
        client.answerRematch(true);
    });
    host.requestRematch();
    CHECK(waitUntil([&]() { return requested; }, 4000), "client got rematchRequested");
    CHECK(waitUntil([&]() {
        return hostGame.moveCount() == 0 && clientGame.moveCount() == 0 &&
               host.state() == OnlineState::Playing && client.state() == OnlineState::Playing;
    }, 4000), "both reset to Playing after rematch");

    host.leaveSession();
    client.leaveSession();
}

// 测试 E：客户端主动离开，主机感知断线
static void testDisconnect() {
    std::printf("--- Test E: disconnect ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    bool gone = false;
    QObject::connect(&host, &OnlineSession::opponentDisconnected, [&]() { gone = true; });

    CHECK(host.startHost(0), "host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "both reach Playing");
    CHECK(host.localMove(7, 7), "host localMove");
    CHECK(waitUntil([&]() { return clientGame.moveCount() == 1; }, 4000), "client move 1");

    client.leaveSession();
    CHECK(waitUntil([&]() { return gone; }, 4000), "host observed opponentDisconnected");

    host.leaveSession();
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    testProtocol();
    testConnectAndSync();
    testIllegalMoveRejected();
    testWinAndRematch();
    testDisconnect();
    std::printf("====================================\n");
    std::printf("TOTAL checks: %d  FAILS: %d\n", g_checks, g_fails);
    std::fflush(stdout);
    return g_fails == 0 ? 0 : 1;
}
