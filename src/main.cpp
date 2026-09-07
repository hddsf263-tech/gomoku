#include <QApplication>
#include <QMouseEvent>
#include <QStringList>
#include <QTimer>

#include <cstdio>

#include "MainWindow.h"
#include "BoardWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("GomokuTeam");
    QCoreApplication::setApplicationName("Gomoku");
    QCoreApplication::setApplicationVersion("2.0");

    Gomoku::MainWindow window;
    window.show();

    const QStringList args = app.arguments();

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
