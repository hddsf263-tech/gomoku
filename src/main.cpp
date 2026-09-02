#include <QApplication>
#include "MainWindow.h"

// Author: [组员姓名待填写]
// Module: main
// Description: 应用程序入口

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    QCoreApplication::setOrganizationName("GomokuTeam");
    QCoreApplication::setApplicationName("Gomoku");
    QCoreApplication::setApplicationVersion("1.0");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
