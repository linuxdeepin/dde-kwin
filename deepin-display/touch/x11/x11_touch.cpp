#include "x11_touch.h"
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


X11Touch:: X11Touch(QObject *parent)
    : AbstractTouch(parent)
{
    new TouchAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Touch"), this, QDBusConnection::ExportAllContents);
}

X11Touch::~ X11Touch()
{
}

bool X11Touch::getDisableIfTypeing()
{
    return true;
}

void X11Touch::setDisableIfTypeing(bool DisableIfTypeing)
{
}

bool X11Touch::getEdgeScroll()
{
    return true;
}

void X11Touch::setEdgeScroll(bool EdgeScroll)
{

}

bool X11Touch::getExist()
{
    return true;
}

bool X11Touch::getHorizScroll()
{
    return true;
}

void X11Touch::setHorizScroll(bool HorizScroll)
{

}

bool X11Touch::getLeftHanded()
{
    return true;
}

void X11Touch::setLeftHanded(bool LeftHanded)
{

}

bool X11Touch::getNaturalScroll()
{
    return true;
}

void X11Touch::setNaturalScroll(bool NaturalScroll)
{

}

bool X11Touch::getPalmDetect()
{
    return true;
}

void X11Touch::setPalmDetect(bool PalmDetect)
{

}

bool X11Touch::getTPadEnable()
{
    return true;
}

void X11Touch::setTPadEnable(bool TPadEnable)
{

}

bool X11Touch::getTapClick()
{
    return true;
}

void X11Touch::setTapClick(bool TapClick)
{

}

bool X11Touch::getVertScroll()
{
    return true;
}

void X11Touch::setVertScroll(bool VertScroll)
{

}

double X11Touch::getMotionAcceleration()
{
    return 1.0;
}

void X11Touch::setMotionAcceleration(double MotionAcceleration)
{

}

double X11Touch::getMotionScaling()
{
    return 1.0;
}

void X11Touch::setMotionScaling(double MotionScaling)
{

}

double X11Touch::getMotionThreshold()
{
    return 1.0;
}

void X11Touch::setMotionThreshold(double MotionThreshold)
{

}

int X11Touch::getDeltaScroll()
{
    return 1;
}

void X11Touch::setDeltaScroll(int DeltaScroll)
{

}

int X11Touch::getDoubleClick()
{
    return 1;
}

void X11Touch::setDoubleClick(int DoubleClick)
{

}

int X11Touch::getDragThreshold()
{
    return 1;
}

void X11Touch::setDragThreshold(int DragThreshold)
{

}

int X11Touch::getPalmMinWidth()
{
    return 1;
}

void X11Touch::setPalmMinWidth(int PalmMinWidth)
{

}

int X11Touch::getPalmMinZ()
{
    return 1;
}

void X11Touch::setPalmMinZ(int PalmMinZ)
{

}

QString X11Touch::getDeviceList()
{
    return "";
}

void X11Touch::Reset()
{

}
