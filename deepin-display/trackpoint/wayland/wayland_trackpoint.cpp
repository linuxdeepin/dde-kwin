#include "wayland_trackpoint.h"

#include <QDebug>

WaylandTrackpoint::WaylandTrackpoint(QObject *parent)
    : AbstractTrackpoint(parent)
{
}

WaylandTrackpoint::~WaylandTrackpoint()
{

}

QString WaylandTrackpoint::DeviceList()
{
    return m_DeviceList;
}

void WaylandTrackpoint::setDeviceList(QString)
{

}

bool WaylandTrackpoint::Exist()
{
    m_Exist = false;
    return m_Exist;
}

void WaylandTrackpoint::setExist(bool)
{

}

bool WaylandTrackpoint::LeftHanded()
{
    m_LeftHanded = false;
    return m_LeftHanded;
}

void WaylandTrackpoint::setLeftHanded(bool)
{

}

bool WaylandTrackpoint::MiddleButtonEmulation()
{
    m_MiddleButtonEmulation = false;
    return m_MiddleButtonEmulation;
}

void WaylandTrackpoint::setMiddleButtonEmulation(bool)
{

}

int WaylandTrackpoint::MiddleButtonTimeout()
{
    m_MiddleButtonTimeout = false;
    return m_MiddleButtonTimeout;
}

void WaylandTrackpoint::setMiddleButtonTimeout(int)
{

}

double WaylandTrackpoint::MotionAcceleration()
{

    return m_MotionAcceleration;
}

void WaylandTrackpoint::setMotionAcceleration(double)
{

}

double WaylandTrackpoint::MotionScaling()
{
    return m_MotionScaling;
}

void WaylandTrackpoint::setMotionScaling(double)
{

}

double WaylandTrackpoint::MotionThreshold()
{
    return m_MotionThreshold;
}

void WaylandTrackpoint::setMotionThreshold(double)
{

}

bool WaylandTrackpoint::WheelEmulation()
{
    m_WheelEmulation= false;
    return m_WheelEmulation;
}

void WaylandTrackpoint::setWheelEmulation(bool)
{

}

int WaylandTrackpoint::WheelEmulationButton()
{
    return m_WheelEmulationButton;
}

void WaylandTrackpoint::setWheelEmulationButton(int)
{

}

int WaylandTrackpoint::WheelEmulationTimeout()
{
    return m_WheelEmulationTimeout;
}

void WaylandTrackpoint::setWheelEmulationTimeout(int)
{

}

bool WaylandTrackpoint::WheelHorizScroll()
{
    m_WheelHorizScroll = false;
    return m_WheelHorizScroll;
}

void WaylandTrackpoint::setWheelHorizScroll(bool)
{

}

void WaylandTrackpoint::Reset()
{

}