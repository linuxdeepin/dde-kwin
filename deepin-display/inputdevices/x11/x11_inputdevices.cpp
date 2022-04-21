#include "x11_inputdevices.h"

#include <QDebug>


X11InputDevices::X11InputDevices(QObject *parent)
    : AbstractInputDevices(parent)
{
}

X11InputDevices::~X11InputDevices()
{
}

unsigned int X11InputDevices::WheelSpeed()
{
    return m_WheelSpeed;

}
void X11InputDevices::setWheelSpeed(unsigned int)
{

}
QList<QDBusObjectPath> X11InputDevices::Touchscreens()
{
    return m_Touchscreens;

} 
void X11InputDevices::setTouchscreens(QList<QDBusObjectPath>)
{

}