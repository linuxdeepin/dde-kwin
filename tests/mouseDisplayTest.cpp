#include "wayland/wayland_mouse.h"
#include "x11/x11_mouse.h"

#include <QGuiApplication>
#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include <QDebug>

#define Service "com.deepin.kwin.display"
#define Path "/Mouse"
#define Interface "com.deepin.KWin.Display.Mouse"

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
    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
    QDBusInterface mouseInterface(Service, Path, "com.deepin.KWin.Display.Mouse", QDBusConnection::sessionBus());
    if (!mouseInterface.isValid()) {
        qDebug() << "com.deepin.KWin.Display.Mouse is not valid!";
        return -1;
    }
    qDebug() << "Exist:" << mouseInterface.property("Exist").toBool();
    qDebug() << "DeviceList:" << mouseInterface.property("DeviceList").toString();
    QDBusReply<void> reply = mouseInterface.call("Reset");
    if (reply.isValid()) {
        qDebug() << "Reset call successfully!";
    } else {
        qDebug() << "Reset call failed!";
    }

    return 0;
}
