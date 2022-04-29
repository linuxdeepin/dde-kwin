#include "wayland_touch.h"
#include "touchadaptor.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>


WaylandTouch::WaylandTouch(QObject *parent)
    : AbstractTouch(parent)
{
    new TouchAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Touch"), this, QDBusConnection::ExportAllContents);
}
WaylandTouch::~WaylandTouch()
{
}

bool WaylandTouch::getDisableIfTypeing()
{
    return true;
}

void WaylandTouch::setDisableIfTypeing(bool DisableIfTypeing)
{
}

bool WaylandTouch::getEdgeScroll()
{
    return true;
}

void WaylandTouch::setEdgeScroll(bool EdgeScroll)
{

}

bool WaylandTouch::getExist()
{
    return true;
}

bool WaylandTouch::getHorizScroll()
{
    return true;
}

void WaylandTouch::setHorizScroll(bool HorizScroll)
{

}

bool WaylandTouch::getLeftHanded()
{
    return true;
}

void WaylandTouch::setLeftHanded(bool LeftHanded)
{

}

bool WaylandTouch::getNaturalScroll()
{
    return true;
}

void WaylandTouch::setNaturalScroll(bool NaturalScroll)
{

}

bool WaylandTouch::getPalmDetect()
{
    return true;
}

void WaylandTouch::setPalmDetect(bool PalmDetect)
{

}

bool WaylandTouch::getTPadEnable()
{
    return true;
}

void WaylandTouch::setTPadEnable(bool TPadEnable)
{

}

bool WaylandTouch::getTapClick()
{
    return true;
}

void WaylandTouch::setTapClick(bool TapClick)
{

}

bool WaylandTouch::getVertScroll()
{
    return true;
}

void WaylandTouch::setVertScroll(bool VertScroll)
{

}

double WaylandTouch::getMotionAcceleration()
{
    return 1.0;
}

void WaylandTouch::setMotionAcceleration(double MotionAcceleration)
{

}

double WaylandTouch::getMotionScaling()
{
    return 1.0;
}

void WaylandTouch::setMotionScaling(double MotionScaling)
{

}

double WaylandTouch::getMotionThreshold()
{
    return 1.0;
}

void WaylandTouch::setMotionThreshold(double MotionThreshold)
{

}

int WaylandTouch::getDeltaScroll()
{
    return 1;
}

void WaylandTouch::setDeltaScroll(int DeltaScroll)
{

}

int WaylandTouch::getDoubleClick()
{
    return 1;
}

void WaylandTouch::setDoubleClick(int DoubleClick)
{

}

int WaylandTouch::getDragThreshold()
{
    return 1;
}

void WaylandTouch::setDragThreshold(int DragThreshold)
{

}

int WaylandTouch::getPalmMinWidth()
{
    return 1;
}

void WaylandTouch::setPalmMinWidth(int PalmMinWidth)
{

}

int WaylandTouch::getPalmMinZ()
{
    return 1;
}

void WaylandTouch::setPalmMinZ(int PalmMinZ)
{

}

QString WaylandTouch::getDeviceList()
{
    return "";
}

void WaylandTouch::Reset()
{

}
