#include "mainwindow.h"
#include "appversion.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral(DEVICE_LINK_APP_NAME));
    app.setApplicationVersion(QStringLiteral(DEVICE_LINK_APP_VERSION));
    app.setOrganizationName(QStringLiteral("DeviceLink"));
    MainWindow window;
    window.show();
    return app.exec();
}
