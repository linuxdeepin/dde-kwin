#include "wayland_mouse.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>


WaylandMouse::WaylandMouse(QObject *parent)
    : AbstractMouse(parent)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral(MouseDBusPath), this, QDBusConnection::ExportScriptableContents);
}

WaylandMouse::~WaylandMouse()
{
}


bool WaylandMouse::Exist()
{
    return true;
}

bool WaylandMouse::AdaptiveAccelProfile()
{
    return true;
}

bool WaylandMouse::DisableTpad()
{
    return true;
}

bool WaylandMouse::LeftHanded()
{
    return true;
}

bool WaylandMouse::MiddleButtonEmulation()
{
    return true;
}

bool WaylandMouse::NaturalScroll()
{
    return true;
}

double WaylandMouse::MotionAcceleration()
{
    return true;
}

double WaylandMouse::MotionScaling()
{
    return true;
}

double WaylandMouse::MotionThreshold()
{
    return true;
}

int WaylandMouse::DoubleClick()
{
    return true;
}

int WaylandMouse::DragThreshold()
{
    return true;
}

QString WaylandMouse::DeviceList()
{
    return "wayland mouse";
}

void WaylandMouse::Reset()
{
}