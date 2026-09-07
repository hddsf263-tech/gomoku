#include <QApplication>
#include <QStringList>
#include <QTimer>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("GomokuTeam");
    QCoreApplication::setApplicationName("Gomoku");
    QCoreApplication::setApplicationVersion("2.0");

    Gomoku::MainWindow window;
    window.show();

    const QStringList args = app.arguments();
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
