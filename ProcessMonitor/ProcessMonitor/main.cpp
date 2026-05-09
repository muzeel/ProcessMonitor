#include "ProcessMonitor.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ProcessMonitor window;
    window.show();
    return app.exec();
}
