#include <QApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QRadioButton>
#include <QSpinBox>
#include <QStringList>
#include <QTimer>

#include <cstdio>

#include "MainWindow.h"
#include "BoardWidget.h"
#include "SoundManager.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("GomokuTeam");
    QCoreApplication::setApplicationName("Gomoku");
    QCoreApplication::setApplicationVersion("2.0");

    const QStringList startupArgs = app.arguments();
    if (startupArgs.contains("--net-loopback")) {
        int port = 25678;
        const int portIndex = startupArgs.indexOf("--port");
        if (portIndex >= 0 && portIndex + 1 < startupArgs.size()) {
            port = startupArgs.at(portIndex + 1).toInt();
        }

        Gomoku::NetworkManager host;
        Gomoku::NetworkManager client;
        bool hostUp = false;
        bool clientUp = false;

        host.onConnected([&]() {
            hostUp = true;
            if (clientUp) {
                QTimer::singleShot(120, [&host]() {
                    host.sendMove(7, 7);
                });
            }
        });
        client.onConnected([&]() {
            clientUp = true;
            if (hostUp) {
                QTimer::singleShot(120, [&host]() {
                    host.sendMove(7, 7);
                });
            }
        });
        client.onMove([&](int row, int col) {
            if (row == 7 && col == 7) {
                client.sendMove(6, 6);
            } else {
                std::fprintf(stdout, "NET_LOOPBACK FAIL unexpected client move\n");
                std::fflush(stdout);
                app.exit(2);
            }
        });
        host.onMove([&](int row, int col) {
            const bool ok = row == 6 && col == 6;
            std::fprintf(stdout, "NET_LOOPBACK %s H8 -> G7\n",
                         ok ? "OK" : "FAIL");
            std::fflush(stdout);
            app.exit(ok ? 0 : 2);
        });
        host.onDisconnected([&]() {
            std::fprintf(stdout, "NET_LOOPBACK FAIL host disconnected\n");
            std::fflush(stdout);
            app.exit(2);
        });
        client.onDisconnected([&]() {
            std::fprintf(stdout, "NET_LOOPBACK FAIL client disconnected\n");
            std::fflush(stdout);
            app.exit(2);
        });
        host.onError([&](const QString& message) {
            std::fprintf(stdout, "NET_LOOPBACK FAIL host: %s\n",
                         message.toUtf8().constData());
            std::fflush(stdout);
            app.exit(2);
        });
        client.onError([&](const QString& message) {
            std::fprintf(stdout, "NET_LOOPBACK FAIL client: %s\n",
                         message.toUtf8().constData());
            std::fflush(stdout);
            app.exit(2);
        });

        if (!host.listen(static_cast<quint16>(port))) {
            std::fprintf(stdout, "NET_LOOPBACK FAIL listen\n");
            std::fflush(stdout);
            return 2;
        }
        client.connectToHost("127.0.0.1", static_cast<quint16>(port));
        QTimer::singleShot(8000, [&app]() {
            std::fprintf(stdout, "NET_LOOPBACK FAIL timeout\n");
            std::fflush(stdout);
            app.exit(2);
        });
        return app.exec();
    }

    if (startupArgs.contains("--sound-smoke")) {
        Gomoku::SoundManager sounds;
        sounds.setPlaceSound("wood");
        sounds.setWinSound("chord");
        QTimer::singleShot(600, [&sounds]() {
            sounds.playPlace(Gomoku::Piece::Black);
        });
        QTimer::singleShot(650, [&sounds]() {
            sounds.playPlace(Gomoku::Piece::White);
        });
        QTimer::singleShot(900, [&sounds]() {
            sounds.playPlace(Gomoku::Piece::Black);
            sounds.playPlace(Gomoku::Piece::White);
            sounds.playWin();
        });
        QTimer::singleShot(3000, [&app, &sounds]() {
            const int ready = sounds.readyBuiltinCount();
            const bool ok = ready == 7;
            std::fprintf(stdout, "SOUND_SMOKE %s ready=%d/7\n",
                         ok ? "OK" : "FAIL", ready);
            std::fflush(stdout);
            app.exit(ok ? 0 : 2);
        });
        return app.exec();
    }

    Gomoku::MainWindow window;
    window.show();

    const QStringList args = startupArgs;

    if (args.contains("--net-ui")) {
        Gomoku::MainWindow clientWindow;
        clientWindow.show();
        const int uiPort = [&args]() {
            const int index = args.indexOf("--port");
            return index >= 0 && index + 1 < args.size()
                ? args.at(index + 1).toInt()
                : 25800;
        }();

        QTimer::singleShot(300, [&]() {
            const auto clickByObjectName = [](QWidget* root,
                                               const QString& name,
                                               bool checkable = false,
                                               bool checked = false) {
                const auto children = root->findChildren<QWidget*>();
                for (QWidget* widget : children) {
                    if (widget->objectName() == name) {
                        auto* button = qobject_cast<QAbstractButton*>(widget);
                        if (!button) {
                            return;
                        }
                        if (checkable) {
                            button->setChecked(checked);
                        }
                        button->click();
                        return;
                    }
                }
            };
            const auto findNetAction = [](QWidget* root) -> QPushButton* {
                const auto buttons = root->findChildren<QPushButton*>();
                for (QPushButton* button : buttons) {
                    if (button->property("netAction").toBool()) {
                        return button;
                    }
                }
                return nullptr;
            };
            const auto findSpin = [](QWidget* root) -> QSpinBox* {
                return root->findChild<QSpinBox*>("netPort");
            };

            clickByObjectName(&window, "modeNet");
            clickByObjectName(&clientWindow, "modeNet");
            clickByObjectName(&window, "netHost");
            clickByObjectName(&clientWindow, "netClient");
            findSpin(&window)->setValue(uiPort);
            findSpin(&clientWindow)->setValue(uiPort);
            if (findNetAction(&window)) {
                findNetAction(&window)->click();
            }
            if (findNetAction(&clientWindow)) {
                findNetAction(&clientWindow)->click();
            }
        });

        QTimer::singleShot(2500, [&]() {
            const QString hostText =
                window.findChild<QLabel*>("netStatus")->text();
            const QString clientText =
                clientWindow.findChild<QLabel*>("netStatus")->text();
            if (!hostText.contains("对手已加入") ||
                !clientText.contains("已连接到主机")) {
                std::fprintf(stdout, "NET_UI FAIL connect %s | %s\n",
                             hostText.toUtf8().constData(),
                             clientText.toUtf8().constData());
                std::fflush(stdout);
                QApplication::exit(2);
                return;
            }

            Gomoku::BoardWidget* hostBoard =
                window.findChild<Gomoku::BoardWidget*>("board");
            const QPoint center = hostBoard->rect().center();
            QMouseEvent press(QEvent::MouseButtonPress, center,
                              hostBoard->mapToGlobal(center),
                              Qt::LeftButton, Qt::LeftButton,
                              Qt::NoModifier);
            QMouseEvent release(QEvent::MouseButtonRelease, center,
                                hostBoard->mapToGlobal(center),
                                Qt::LeftButton, Qt::NoButton,
                                Qt::NoModifier);
            QApplication::sendEvent(hostBoard, &press);
            QApplication::sendEvent(hostBoard, &release);
        });

        QTimer::singleShot(3500, [&]() {
            Gomoku::BoardWidget* hostBoard =
                window.findChild<Gomoku::BoardWidget*>("board");
            Gomoku::BoardWidget* clientBoard =
                clientWindow.findChild<Gomoku::BoardWidget*>("board");
            const QString hostText =
                window.findChild<QLabel*>("moveText")->text();
            const QString clientText =
                clientWindow.findChild<QLabel*>("moveText")->text();
            if (!hostText.startsWith("1 手") ||
                !clientText.startsWith("1 手")) {
                std::fprintf(stdout, "NET_UI FAIL first move %s | %s\n",
                             hostText.toUtf8().constData(),
                             clientText.toUtf8().constData());
                std::fflush(stdout);
                QApplication::exit(2);
                return;
            }
            const QPoint point = clientBoard->rect().center() + QPoint(64, 0);
            QMouseEvent press(QEvent::MouseButtonPress, point,
                              clientBoard->mapToGlobal(point),
                              Qt::LeftButton, Qt::LeftButton,
                              Qt::NoModifier);
            QMouseEvent release(QEvent::MouseButtonRelease, point,
                                clientBoard->mapToGlobal(point),
                                Qt::LeftButton, Qt::NoButton,
                                Qt::NoModifier);
            QApplication::sendEvent(clientBoard, &press);
            QApplication::sendEvent(clientBoard, &release);
        });

        QTimer::singleShot(4500, [&]() {
            const QString hostText =
                window.findChild<QLabel*>("moveText")->text();
            const QString clientText =
                clientWindow.findChild<QLabel*>("moveText")->text();
            const bool ok = hostText.startsWith("2 手") &&
                            clientText.startsWith("2 手");
            std::fprintf(stdout, "NET_UI %s %s | %s\n",
                         ok ? "OK" : "FAIL",
                         hostText.toUtf8().constData(),
                         clientText.toUtf8().constData());
            std::fflush(stdout);
            QApplication::exit(ok ? 0 : 2);
        });

        return app.exec();
    }

    if (args.contains("--ai-smoke")) {
        QTimer::singleShot(500, [&window, &app]() {
            QPushButton* modeAI = nullptr;
            const auto buttons = window.findChildren<QPushButton*>();
            for (QPushButton* button : buttons) {
                if (button->objectName() == "modeAI") {
                    modeAI = button;
                    break;
                }
            }
            if (!modeAI) {
                std::fprintf(stdout, "AI_SMOKE FAIL: modeAI not found\n");
                std::fflush(stdout);
                app.exit(2);
                return;
            }
            modeAI->click();

            QTimer::singleShot(300, [&window, &app, modeAI]() {
                auto* board = window.findChild<Gomoku::BoardWidget*>("board");
                auto* status = window.findChild<QLabel*>("statusText");
                std::fprintf(stdout, "AI_SMOKE state checked=%d status=%s board=%d rect=%dx%d\n",
                             modeAI->isChecked(),
                             status ? status->text().toUtf8().constData() : "none",
                             board ? 1 : 0,
                             board ? board->width() : 0,
                             board ? board->height() : 0);
                std::fflush(stdout);
                if (!board) {
                    std::fprintf(stdout, "AI_SMOKE FAIL: board not found\n");
                    std::fflush(stdout);
                    app.exit(2);
                    return;
                }
                const QPoint center = board->rect().center();
                QMouseEvent press(QEvent::MouseButtonPress, center,
                                  board->mapToGlobal(center),
                                  Qt::LeftButton, Qt::LeftButton,
                                  Qt::NoModifier);
                QApplication::sendEvent(board, &press);
                QMouseEvent release(QEvent::MouseButtonRelease, center,
                                    board->mapToGlobal(center),
                                    Qt::LeftButton, Qt::NoButton,
                                    Qt::NoModifier);
                QApplication::sendEvent(board, &release);

                QTimer::singleShot(4500, [&window, &app]() {
                    auto* moveText = window.findChild<QLabel*>("moveText");
                    const QString text = moveText ? moveText->text() : QString();
                    const bool ok = text.startsWith("2 手");
                    std::fprintf(stdout, "AI_SMOKE %s %s\n",
                                 ok ? "OK" : "FAIL",
                                 text.toUtf8().constData());
                    std::fflush(stdout);
                    app.exit(ok ? 0 : 2);
                });
            });
        });
        return app.exec();
    }

    const bool netHostSmoke = args.contains("--net-host");
    const bool netClientSmoke = args.contains("--net-client");
    if (netHostSmoke || netClientSmoke) {
        int port = 23456;
        const int portIndex = args.indexOf("--port");
        if (portIndex >= 0 && portIndex + 1 < args.size()) {
            port = args.at(portIndex + 1).toInt();
        }
        QTimer::singleShot(500, [&window, &app, netHostSmoke, port]() {
            auto* modeNet = window.findChild<QPushButton*>("modeNet");
            auto* netPort = window.findChild<QSpinBox*>("netPort");
            QPushButton* netAction = nullptr;
            const auto allButtons = window.findChildren<QPushButton*>();
            for (QPushButton* button : allButtons) {
                if (button->property("netAction").toBool()) {
                    netAction = button;
                    break;
                }
            }
            auto* roleButton = netHostSmoke
                ? window.findChild<QRadioButton*>("netHost")
                : window.findChild<QRadioButton*>("netClient");
            if (modeNet) {
                modeNet->click();
            }
            if (netPort) {
                netPort->setValue(port);
            }
            if (roleButton) {
                roleButton->click();
            }
            if (netAction) {
                netAction->click();
            }

            // Check both sides early, but keep the host alive a little longer
            // so the client can finish reading its status first.
            const int checkDelayMs = netHostSmoke ? 2500 : 3000;
            QTimer::singleShot(checkDelayMs, [&window, &app, netHostSmoke]() {
                auto* status = window.findChild<QLabel*>("netStatus");
                const QString text = status ? status->text() : QString();
                const bool ok = netHostSmoke
                    ? text.contains("对手已加入")
                    : text.contains("已连接到主机");
                if (!ok) {
                    std::fprintf(stdout, "NET_SMOKE FAIL %s\n",
                                 text.toUtf8().constData());
                    std::fflush(stdout);
                    app.exit(2);
                    return;
                }
                std::fprintf(stdout, "NET_SMOKE OK %s\n",
                             text.toUtf8().constData());
                std::fflush(stdout);
                if (netHostSmoke) {
                    QTimer::singleShot(9000, [&app]() {
                        app.exit(0);
                    });
                } else {
                    app.exit(0);
                }
            });
        });
        return app.exec();
    }

    const int shotIndex = args.indexOf("--screenshot");
    if (shotIndex >= 0 && shotIndex + 1 < args.size()) {
        const QString path = args.at(shotIndex + 1);
        QTimer::singleShot(900, [&window, &app, path]() {
            window.grab().save(path);
            app.quit();
        });
    }
    return app.exec();
}
