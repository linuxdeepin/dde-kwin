#include "wayland/wayland_mouse.h"
#include "x11/x11_mouse.h"

#include <QGuiApplication>
#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include <DLog>

#define Service "com.deepin.kwin.display"
#define Path "/com/deepin/kwin/display"
#define Interface "com.deepin.kwin.display"

DCORE_USE_NAMESPACE

//在wayland下，kwindowsystem默认使用wayland平台的接口，但目前wayland的接口currentDesktop()未实现，所以设置环境变量转为使用xcb平台的接口
__attribute__((constructor))void init()
{
    qputenv("QT_QPA_PLATFORM", "xcb");
}

bool isX11Platform()
{
    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));

    if (XDG_SESSION_TYPE != QLatin1String("x11")) {
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("DeepinDisplay");

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();


    if (isX11Platform()) {
        new X11Mouse();
    } else {
        new WaylandMouse();
    }

    if (!QDBusConnection::sessionBus().registerService(Service)) {
        return -1;
    }

    return app.exec();
}
