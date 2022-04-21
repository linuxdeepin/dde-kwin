#include "wayland_inputdevices.h"

#include <QDebug>

WaylandInputDevices::WaylandInputDevices(QObject *parent)
    : AbstractInputDevices(parent)
{
}

WaylandInputDevices::~WaylandInputDevices()
{

}

unsigned int WaylandInputDevices::WheelSpeed()
{
    return m_WheelSpeed;

}
void WaylandInputDevices::setWheelSpeed(unsigned int)
{

}
QList<QDBusObjectPath> WaylandInputDevices::Touchscreens()
{
    return m_Touchscreens;

} 
void WaylandInputDevices::setTouchscreens(QList<QDBusObjectPath>)
{

}