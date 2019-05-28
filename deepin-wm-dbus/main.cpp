#include "wmadaptor.h"
#include "deepinwmfaker.h"

#include <QGuiApplication>
#include <DLog>

#define Service "com.deepin.wm"
#define Path "/com/deepin/wm"
#define Interface "com.deepin.wm"

DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("DeepinWMFaker");

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    DeepinWMFaker facker;
    WmAdaptor adapter(&facker);
    Q_UNUSED(adapter)

    if (!QDBusConnection::sessionBus().registerService(Service)) {
        return -1;
    }
    if (!QDBusConnection::sessionBus().registerObject(Path, Interface, &facker)) {
        return -2;
    }

    return app.exec();
}
