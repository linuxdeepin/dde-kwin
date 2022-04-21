#include "x11_trackpoint.h"

#include <QDebug>


X11Trackpoint:: X11Trackpoint(QObject *parent)
    : AbstractTrackpoint(parent)
{
}

X11Trackpoint::~ X11Trackpoint()
{
}

QString X11Trackpoint::DeviceList()
{
    return m_DeviceList;
}
void X11Trackpoint::setDeviceList(QString)
{

}

bool X11Trackpoint::Exist() {
        m_Exist = false;
    qDebug()<<">>>>>> Exist:"<<m_Exist;
    return m_Exist;
} 
void X11Trackpoint::setExist(bool)
{

}

bool X11Trackpoint::LeftHanded()
{
            m_LeftHanded = false;
    qDebug()<<">>>>>> LeftHanded:"<<m_LeftHanded;
    return m_LeftHanded;
} 
void X11Trackpoint::setLeftHanded(bool)
{

}

bool X11Trackpoint::MiddleButtonEmulation() {
        m_MiddleButtonEmulation = false;
    qDebug()<<">>>>>> MiddleButtonEmulation:"<<m_MiddleButtonEmulation;
    return m_MiddleButtonEmulation;
} 
void X11Trackpoint::setMiddleButtonEmulation(bool)
{

}

int X11Trackpoint::MiddleButtonTimeout()
{
    m_MiddleButtonTimeout = false;
    qDebug()<<">>>>>> m_MiddleButtonTimeout:"<<m_MiddleButtonTimeout;
    return m_MiddleButtonTimeout;
} 
void X11Trackpoint::setMiddleButtonTimeout(int)
{

}

double X11Trackpoint::MotionAcceleration()
{

    return m_MotionAcceleration;
} 
void X11Trackpoint::setMotionAcceleration(double)
{

}

double X11Trackpoint::MotionScaling() {
    return m_MotionScaling;
} 
void X11Trackpoint::setMotionScaling(double)
{

}

double X11Trackpoint::MotionThreshold()
{
    return m_MotionThreshold;
} 
void X11Trackpoint::setMotionThreshold(double)
{

}

bool X11Trackpoint::WheelEmulation()
{
    m_WheelEmulation= false;
    qDebug()<<">>>>>> WheelEmulation:"<<m_WheelEmulation;
    return m_WheelEmulation;
} 
void X11Trackpoint::setWheelEmulation(bool)
{

}

int X11Trackpoint::WheelEmulationButton() {
    return m_WheelEmulationButton;
}
void X11Trackpoint::setWheelEmulationButton(int) {

}

int X11Trackpoint::WheelEmulationTimeout()
{
    return m_WheelEmulationTimeout;
} 
void X11Trackpoint::setWheelEmulationTimeout(int)
{

}

bool X11Trackpoint::WheelHorizScroll()
{
    m_WheelHorizScroll = false;
    qDebug()<<">>>>>> WheelHorizScroll:"<<m_WheelHorizScroll;
    return m_WheelHorizScroll;
} 
void X11Trackpoint::setWheelHorizScroll(bool)
{

}

void X11Trackpoint::Reset() { 

}