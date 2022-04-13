#include "x11_mouse.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>


X11Mouse:: X11Mouse(QObject *parent)
    : AbstractMouse(parent)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral(MouseDBusPath), this, QDBusConnection::ExportScriptableContents);
}

X11Mouse::~ X11Mouse()
{
}


bool X11Mouse::Exist()
{
    return true;
}

bool X11Mouse::AdaptiveAccelProfile()
{
    return true;
}

bool X11Mouse::DisableTpad()
{
    return true;
}

bool X11Mouse::LeftHanded()
{
    return true;
}

bool X11Mouse::MiddleButtonEmulation()
{
    return true;
}

bool X11Mouse::NaturalScroll()
{
    return true;
}

double X11Mouse::MotionAcceleration()
{
    return true;
}

double X11Mouse::MotionScaling()
{
    return true;
}

double X11Mouse::MotionThreshold()
{
    return true;
}

int X11Mouse::DoubleClick()
{
    return true;
}

int X11Mouse::DragThreshold()
{
    return true;
}

QString X11Mouse::DeviceList()
{
    return "x11 mouse";
}

void X11Mouse::Reset()
{
}