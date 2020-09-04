#include "wmadaptor.h"
#include "deepinwmfaker.h"

#include <QGuiApplication>
#include <DLog>

#define Service "com.deepin.wm"
#define Path "/com/deepin/wm"
#define Interface "com.deepin.wm"

DCORE_USE_NAMESPACE

//在wayland下，kwindowsystem默认使用wayland平台的接口，但目前wayland的接口currentDesktop()未实现，所以设置环境变量转为使用xcb平台的接口
__attribute__((constructor))void init()
{
    qputenv("QT_QPA_PLATFORM", "xcb");
}

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
