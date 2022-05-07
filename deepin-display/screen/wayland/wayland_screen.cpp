#include "wayland_screen.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>
#include <QDBusReply>

#include "wayland/wayland_screen.h"
#include "screenadaptor.h"

#define DBUS_DEEPIN_WM_SERVICE "org.kde.KWin"
#define DBUS_DEEPIN_WM_OBJ "/KWin"
#define DBUS_DEEPIN_WM_INTF "org.kde.KWin"

#define screenPath "/com/deepin/kwin/Display/Screen"
#define screenInterface "com.deepin.kwin.Display.Screen"

bool isX11Platform()
{
    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));

    if (XDG_SESSION_TYPE != QLatin1String("x11")) {
        return false;
    }
    return true;
}

WaylandScreen::WaylandScreen(QObject *parent)
    : AbstractScreen(parent)
{
    new ScreenAdaptor(this);

    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);
    QDBusReply<QString> getReply = wm.call("getTouchDeviceToScreenInfo");
    if(!getReply.value().isEmpty()) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(getReply.value().toUtf8(), &error);
        if (QJsonParseError::NoError == error.error) {
            QList<QVariant> list = document.toVariant().toList();
            for (int i = 0; i < list.count(); i++) {
                QVariant item = list.at(i);
                QVariantMap map = item.toMap();
                int nScreenId = map["ScreenId"].toInt();
                QString strUUId = map["ScreenUuid"].toString();
                QString strTouchDeviceNode = map.find("TouchDevice")->toString();
                QString str = QString(screenPath).append("%1""%2").arg("_").arg(nScreenId);
                QDBusConnection::sessionBus().registerObject(str, this, QDBusConnection::ExportAllContents);
            }
        }
    }
}

WaylandScreen::~ WaylandScreen()
{

}

QVariantList WaylandScreen::Modes()
{
    return QVariantList();
}

QVariantList WaylandScreen::PreferredModes()
{
    return QVariantList();
}

QVariantList WaylandScreen::Reflects()
{
    return QVariantList();
}

QVariantList WaylandScreen::Rotations()
{
    return QVariantList();
}

bool WaylandScreen::Connected()
{
    return true;
}

bool WaylandScreen::Enabled()
{
    return true;
}

uchar WaylandScreen::CurrentRotateMode()
{
    return uchar();
}

double WaylandScreen::Brightness()
{
    return double();
}

double WaylandScreen::RefreshRate()
{
    return double();
}

int WaylandScreen::X()
{
    return 0;
}

int WaylandScreen::Y()
{
    return 0;
}

QString WaylandScreen::Manufacturer()
{
    return QString();
}

QString WaylandScreen::Model()
{
    return QString();
}

QString WaylandScreen::Name()
{
    return QString();
}

QVariantList WaylandScreen::BestMode()
{
    return QVariantList();
}

QVariantList WaylandScreen::CurrentMode()
{
    return QVariantList();
}

int WaylandScreen::Height()
{
    return 0;
}

int WaylandScreen::Reflect()
{
    return 0;
}

int WaylandScreen::Rotation()
{
    return 0;
}

int WaylandScreen::Width()
{
    return 0;
}

int WaylandScreen::ID()
{
    return 0;
}

int WaylandScreen::MmHeight()
{
    return 0;
}

int WaylandScreen::MmWidth()
{
    return 0;
}


void WaylandScreen::Enable(bool isEnable)
{

}

void WaylandScreen::SetMode(int nMode)
{

}

void WaylandScreen::SetModeBySize(int nWidth, int nHeight)
{

}

void WaylandScreen::SetPosition(int x,int y)
{

}

void WaylandScreen::SetReflect(int nReflect)
{

}

void WaylandScreen::SetRefreshRate(double dRefreshRate)
{

}

void WaylandScreen::SetRotation(int nRotation)
{

}
