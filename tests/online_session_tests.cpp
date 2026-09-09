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

// 测试 F/G/H：非法落子类型（越界 / 非回合 / 结束后）
// 测试 F/G：非法落子（越界 / 非个人回合）
static void testIllegalMoveKinds() {
    std::printf("--- Test F/G: illegal move kinds ---\n");

    // F: 越界。先由黑方落子轮到白方，白方再请求越界坐标。
    {
        GameEngine hostGame, clientGame;
        OnlineSession host(hostGame);
        OnlineSession client(clientGame);
        QString reason;
        QObject::connect(&client, &OnlineSession::moveRejected,
                         [&](const QString& r) { reason = r; });
        CHECK(host.startHost(0), "F host startHost(0)");
        const quint16 port = host.port();
        client.connectToHost(QStringLiteral("127.0.0.1"), port);
        CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                    && client.state() == OnlineState::Playing; }, 6000),
              "F both reach Playing");
        CHECK(host.localMove(7, 7), "F host black move");
        CHECK(waitUntil([&]() { return clientGame.moveCount() == 1; }, 4000), "F client synced");
        CHECK(client.localMove(99, 99), "F white sends out-of-range request");
        CHECK(waitUntil([&]() { return !reason.isEmpty(); }, 4000), "F client received REJECT");
        CHECK(reason == QStringLiteral("INVALID_POSITION"), "F reject reason=INVALID_POSITION");
        CHECK(hostGame.moveCount() == 1, "F host did not commit out-of-range");
        host.leaveSession();
        client.leaveSession();
    }

    // G: 非个人回合。当前轮到黑方，白方尝试落子应在本地被拒绝（不发送）。
    {
        GameEngine hostGame, clientGame;
        OnlineSession host(hostGame);
        OnlineSession client(clientGame);
        CHECK(host.startHost(0), "G host startHost(0)");
        const quint16 port = host.port();
        client.connectToHost(QStringLiteral("127.0.0.1"), port);
        CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                    && client.state() == OnlineState::Playing; }, 6000),
              "G both reach Playing");
        CHECK(!client.localMove(8, 8), "G white localMove rejected locally (not turn)");
        CHECK(hostGame.moveCount() == 0, "G host board unchanged");
        CHECK(clientGame.moveCount() == 0, "G client board unchanged");
        host.leaveSession();
        client.leaveSession();
    }

    // H: 游戏结束后不能再落子（客户端处于 GameOver，localMove 返回 false）
    {
        GameEngine hostGame, clientGame;
        OnlineSession host(hostGame);
        OnlineSession client(clientGame);
        CHECK(host.startHost(0), "H host startHost(0)");
        const quint16 port = host.port();
        client.connectToHost(QStringLiteral("127.0.0.1"), port);
        CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                    && client.state() == OnlineState::Playing; }, 6000),
              "H both reach Playing");
        const int blackMoves[5][2] = {{7,3},{7,4},{7,5},{7,6},{7,7}};
        const int whiteMoves[4][2] = {{0,0},{0,1},{0,2},{0,3}};
        for (int i = 0; i < 4; i++) {
            host.localMove(blackMoves[i][0], blackMoves[i][1]);
            waitUntil([&]() { return clientGame.moveCount() == hostGame.moveCount(); }, 4000);
            client.localMove(whiteMoves[i][0], whiteMoves[i][1]);
            waitUntil([&]() { return hostGame.moveCount() == (2 * i + 2); }, 4000);
        }
        host.localMove(blackMoves[4][0], blackMoves[4][1]);
        CHECK(waitUntil([&]() { return client.state() == OnlineState::GameOver; }, 4000),
              "H client reached GameOver");
        CHECK(!client.localMove(6, 6), "H client cannot move after game over");
        host.leaveSession();
        client.leaveSession();
    }
}// 测试 I：纵向五连获胜广播（网络 GAME_OVER）
static void testVerticalWin() {
    std::printf("--- Test I: vertical win broadcast ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    CHECK(host.startHost(0), "I host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "I both reach Playing");
    const int blackMoves[5][2] = {{3,7},{4,7},{5,7},{6,7},{7,7}};
    const int whiteMoves[4][2] = {{0,0},{0,1},{0,2},{0,3}};
    for (int i = 0; i < 4; i++) {
        host.localMove(blackMoves[i][0], blackMoves[i][1]);
        waitUntil([&]() { return clientGame.moveCount() == hostGame.moveCount(); }, 4000);
        client.localMove(whiteMoves[i][0], whiteMoves[i][1]);
        waitUntil([&]() { return hostGame.moveCount() == (2 * i + 2); }, 4000);
    }
    host.localMove(blackMoves[4][0], blackMoves[4][1]);
    CHECK(waitUntil([&]() { return clientGame.moveCount() == 9; }, 4000), "I client synced winning move");
    CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver
                                && client.state() == OnlineState::GameOver; }, 4000),
          "I both reach GameOver");
    CHECK(hostGame.status() == GameStatus::BlackWin, "I host status BlackWin (vertical)");
    CHECK(clientGame.status() == GameStatus::BlackWin, "I client status BlackWin (vertical)");
    host.leaveSession();
    client.leaveSession();
}

// 测试 J：主机主动关闭，客户端感知断线
static void testHostCloses() {
    std::printf("--- Test J: host closes -> client disconnect ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    bool gone = false;
    QObject::connect(&client, &OnlineSession::opponentDisconnected, [&]() { gone = true; });
    CHECK(host.startHost(0), "J host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "J both reach Playing");
    host.leaveSession();
    CHECK(waitUntil([&]() { return gone; }, 4000), "J client observed opponentDisconnected");
    client.leaveSession();
}

// 测试 K：一方拒绝重赛
static void testRematchDeclined() {
    std::printf("--- Test K: rematch declined ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    bool declined = false;
    QObject::connect(&host, &OnlineSession::rematchDeclined, [&]() { declined = true; });
    CHECK(host.startHost(0), "K host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "K both reach Playing");
    const int blackMoves[5][2] = {{7,3},{7,4},{7,5},{7,6},{7,7}};
    const int whiteMoves[4][2] = {{0,0},{0,1},{0,2},{0,3}};
    for (int i = 0; i < 4; i++) {
        host.localMove(blackMoves[i][0], blackMoves[i][1]);
        waitUntil([&]() { return clientGame.moveCount() == hostGame.moveCount(); }, 4000);
        client.localMove(whiteMoves[i][0], whiteMoves[i][1]);
        waitUntil([&]() { return hostGame.moveCount() == (2 * i + 2); }, 4000);
    }
    host.localMove(blackMoves[4][0], blackMoves[4][1]);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver
                                && client.state() == OnlineState::GameOver; }, 4000),
          "K both reach GameOver");
    host.requestRematch();
    client.answerRematch(false);
    CHECK(waitUntil([&]() { return declined; }, 4000), "K host saw rematchDeclined");
    CHECK(hostGame.moveCount() != 0, "K board not reset after decline");
    host.leaveSession();
    client.leaveSession();
}// 测试 L：终局后一方断线，对方进入 OpponentDisconnected
static void testDisconnectAfterGameOver() {
    std::printf("--- Test L: disconnect after game over ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    bool gone = false;
    QObject::connect(&host, &OnlineSession::opponentDisconnected, [&]() { gone = true; });
    CHECK(host.startHost(0), "L host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "L both reach Playing");
    const int blackMoves[5][2] = {{7,3},{7,4},{7,5},{7,6},{7,7}};
    const int whiteMoves[4][2] = {{0,0},{0,1},{0,2},{0,3}};
    for (int i = 0; i < 4; i++) {
        host.localMove(blackMoves[i][0], blackMoves[i][1]);
        waitUntil([&]() { return clientGame.moveCount() == hostGame.moveCount(); }, 4000);
        client.localMove(whiteMoves[i][0], whiteMoves[i][1]);
        waitUntil([&]() { return hostGame.moveCount() == (2 * i + 2); }, 4000);
    }
    host.localMove(blackMoves[4][0], blackMoves[4][1]);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver
                                && client.state() == OnlineState::GameOver; }, 4000),
          "L both reach GameOver");
    client.leaveSession();
    CHECK(waitUntil([&]() { return gone; }, 4000), "L host saw opponentDisconnected after game over");
    host.leaveSession();
}


// 测试 M：Host 投降 -> 白方获胜，Client 感知 resigned
static void testHostResign() {
    std::printf("--- Test M: host resign -> client perceives ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    Piece resignedBy = Piece::Empty;
    GameStatus resignedStatus = GameStatus::InProgress;
    bool clientSawResign = false;
    QObject::connect(&client, &OnlineSession::resigned, [&](Piece r, GameStatus s) {
        resignedBy = r; resignedStatus = s; clientSawResign = true;
    });
    CHECK(host.startHost(0), "M host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "M both reach Playing");
    CHECK(host.localMove(7, 7), "M host black move");
    CHECK(waitUntil([&]() { return clientGame.moveCount() == 1; }, 4000), "M client synced");
    host.resign();
    CHECK(hostGame.status() == GameStatus::WhiteWin, "M host board WhiteWin (host/black resigned)");
    CHECK(waitUntil([&]() { return clientSawResign; }, 4000), "M client saw resigned signal");
    CHECK(resignedBy == Piece::Black, "M resigner=Black");
    CHECK(resignedStatus == GameStatus::WhiteWin, "M client status WhiteWin");
    CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver
                                && client.state() == OnlineState::GameOver; }, 4000),
          "M both reach GameOver");
    CHECK(clientGame.status() == GameStatus::WhiteWin, "M client board WhiteWin");
    host.leaveSession();
    client.leaveSession();
}

// 测试 N：Client 投降 -> 黑方获胜，Host 感知 resigned
static void testClientResign() {
    std::printf("--- Test N: client resign -> host perceives ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    Piece resignedBy = Piece::Empty;
    GameStatus resignedStatus = GameStatus::InProgress;
    bool hostSawResign = false;
    QObject::connect(&host, &OnlineSession::resigned, [&](Piece r, GameStatus s) {
        resignedBy = r; resignedStatus = s; hostSawResign = true;
    });
    CHECK(host.startHost(0), "N host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "N both reach Playing");
    CHECK(host.localMove(7, 7), "N host black move");
    CHECK(waitUntil([&]() { return clientGame.moveCount() == 1; }, 4000), "N client synced");
    client.resign();
    CHECK(clientGame.status() == GameStatus::BlackWin, "N client board BlackWin (client/white resigned)");
    CHECK(waitUntil([&]() { return hostSawResign; }, 4000), "N host saw resigned signal");
    CHECK(resignedBy == Piece::White, "N resigner=White");
    CHECK(resignedStatus == GameStatus::BlackWin, "N host status BlackWin");
    CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver
                                && client.state() == OnlineState::GameOver; }, 4000),
          "N both reach GameOver");
    CHECK(hostGame.status() == GameStatus::BlackWin, "N host board BlackWin");
    host.leaveSession();
    client.leaveSession();
}

// 测试 O：投降后仍可再来一局
static void testResignThenRematch() {
    std::printf("--- Test O: resign then rematch ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    CHECK(host.startHost(0), "O host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "O both reach Playing");
    host.resign();
    CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver
                                && client.state() == OnlineState::GameOver; }, 4000),
          "O both GameOver after resign");
    bool hostSawRematch = false;
    QObject::connect(&host, &OnlineSession::rematchRequested, [&]() {
        hostSawRematch = true;
        host.answerRematch(true);
    });
    client.requestRematch();
    CHECK(waitUntil([&]() { return hostSawRematch; }, 4000), "O host saw rematchRequested");
    CHECK(waitUntil([&]() { return hostGame.moveCount() == 0 && clientGame.moveCount() == 0
                                && host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 4000),
          "O both reset and Playing after rematch");
    host.leaveSession();
    client.leaveSession();
}

// 测试 P：重赛状态机 - 发起方进入 Restarting，拒绝方回 GameOver 不重置
static void testRematchStateMachine() {
    std::printf("--- Test P: rematch state machine ---\n");
    GameEngine hostGame, clientGame;
    OnlineSession host(hostGame);
    OnlineSession client(clientGame);
    CHECK(host.startHost(0), "P host startHost(0)");
    const quint16 port = host.port();
    client.connectToHost(QStringLiteral("127.0.0.1"), port);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                && client.state() == OnlineState::Playing; }, 6000),
          "P both reach Playing");
    // 直接终局（连五）
    const int blackMoves[5][2] = {{7,3},{7,4},{7,5},{7,6},{7,7}};
    const int whiteMoves[4][2] = {{0,0},{0,1},{0,2},{0,3}};
    for (int i = 0; i < 4; i++) {
        host.localMove(blackMoves[i][0], blackMoves[i][1]);
        waitUntil([&]() { return clientGame.moveCount() == hostGame.moveCount(); }, 4000);
        client.localMove(whiteMoves[i][0], whiteMoves[i][1]);
        waitUntil([&]() { return hostGame.moveCount() == (2 * i + 2); }, 4000);
    }
    host.localMove(blackMoves[4][0], blackMoves[4][1]);
    CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver
                                && client.state() == OnlineState::GameOver; }, 4000),
          "P both reach GameOver");

    // 发起方立即进入 Restarting
    host.requestRematch();
    CHECK(host.state() == OnlineState::Restarting, "P host state Restarting after requestRematch");

    // 对方拒绝：不重置，回 GameOver
    bool hostDeclined = false;
    QObject::connect(&host, &OnlineSession::rematchDeclined, [&]() { hostDeclined = true; });
    client.answerRematch(false);
    CHECK(waitUntil([&]() { return hostDeclined; }, 4000), "P host saw rematchDeclined");
    CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver; }, 4000),
          "P host back to GameOver after decline");
    CHECK(hostGame.moveCount() != 0, "P board not reset after decline");
    host.leaveSession();
    client.leaveSession();
}

// 测试 Q：非对局中投降无效 & 重复重赛请求幂等
static void testResignIdempotency() {
    std::printf("--- Test Q: resign idempotency / rematch duplicates ---\n");
    // 未进入对局（未连接）时投降无效
    {
        GameEngine g;
        OnlineSession s(g);
        s.resign();
        CHECK(s.state() == OnlineState::Disconnected, "Q still Disconnected");
        CHECK(g.status() == GameStatus::InProgress, "Q board still InProgress");
    }
    // 对局结束后重复请求重赛不崩溃、状态机不破坏
    {
        GameEngine hostGame, clientGame;
        OnlineSession host(hostGame);
        OnlineSession client(clientGame);
        CHECK(host.startHost(0), "Q host startHost(0)");
        const quint16 port = host.port();
        client.connectToHost(QStringLiteral("127.0.0.1"), port);
        CHECK(waitUntil([&]() { return host.state() == OnlineState::Playing
                                    && client.state() == OnlineState::Playing; }, 6000),
              "Q both reach Playing");
        const int blackMoves[5][2] = {{7,3},{7,4},{7,5},{7,6},{7,7}};
        const int whiteMoves[4][2] = {{0,0},{0,1},{0,2},{0,3}};
        for (int i = 0; i < 4; i++) {
            host.localMove(blackMoves[i][0], blackMoves[i][1]);
            waitUntil([&]() { return clientGame.moveCount() == hostGame.moveCount(); }, 4000);
            client.localMove(whiteMoves[i][0], whiteMoves[i][1]);
            waitUntil([&]() { return hostGame.moveCount() == (2 * i + 2); }, 4000);
        }
        host.localMove(blackMoves[4][0], blackMoves[4][1]);
        CHECK(waitUntil([&]() { return host.state() == OnlineState::GameOver
                                    && client.state() == OnlineState::GameOver; }, 4000),
              "Q both reach GameOver");

        bool clientSawRequest = false;
        QObject::connect(&client, &OnlineSession::rematchRequested, [&]() {
            clientSawRequest = true;
            client.answerRematch(true);
        });
        host.requestRematch();
        host.requestRematch(); // 重复请求应幂等
        CHECK(waitUntil([&]() { return clientSawRequest; }, 4000), "Q client saw rematchRequested");
        CHECK(waitUntil([&]() { return hostGame.moveCount() == 0 && clientGame.moveCount() == 0
                                    && host.state() == OnlineState::Playing
                                    && client.state() == OnlineState::Playing; }, 4000),
              "Q both reset and Playing after duplicate rematch");
        host.leaveSession();
        client.leaveSession();
    }
}
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    testProtocol();
    testConnectAndSync();
    testIllegalMoveRejected();
    testWinAndRematch();
    testDisconnect();
    testIllegalMoveKinds();
    testVerticalWin();
    testHostCloses();
    testRematchDeclined();
    testDisconnectAfterGameOver();
    testHostResign();
    testClientResign();
    testResignThenRematch();
    testRematchStateMachine();
    testResignIdempotency();
    std::printf("====================================\n");
    std::printf("TOTAL checks: %d  FAILS: %d\n", g_checks, g_fails);
    std::fflush(stdout);
    return g_fails == 0 ? 0 : 1;
}


