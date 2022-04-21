#include "wayland_mouse.h"

#include <QDebug>

WaylandMouse::WaylandMouse(QObject *parent)
    : AbstractMouse(parent)
{
}

WaylandMouse::~WaylandMouse()
{
}


bool WaylandMouse::AdaptiveAccelProfile()
{
    m_AdaptiveAccelProfile = false;
    qDebug()<<">>>>>> AdaptiveAccelProfile:"<<m_AdaptiveAccelProfile;
    return m_AdaptiveAccelProfile;
}

void WaylandMouse::setAdaptiveAccelProfile(bool)
{ 

}
QString WaylandMouse::DeviceList()
{ 
    return m_DeviceList;
}
void WaylandMouse::setDeviceList(QString)
{ 

}

bool WaylandMouse::DisableTpad()
{ 
    m_DisableTpad = false;
    qDebug()<<">>>>>> DisableTpad:"<<m_DisableTpad;
    return m_DisableTpad;
}
void WaylandMouse::setDisableTpad(bool)
{ 

}

unsigned int WaylandMouse::DoubleClick()
{ 
    return m_DoubleClick;
}
void WaylandMouse::setDoubleClick(unsigned int)
{ 

}

int WaylandMouse::DragThreshold()
{ 
    return m_DragThreshold;
}
void WaylandMouse::setDragThreshold(int)
{ 

}

bool WaylandMouse::Exist()
{ 
    m_Exist = false;
    return m_Exist;
}
void WaylandMouse::setExist(bool)
{ 

}

bool WaylandMouse::LeftHanded()
{ 
    m_LeftHanded = false;
    return m_LeftHanded;
}
void WaylandMouse::setLeftHanded(bool)
{ 

}

bool WaylandMouse::MiddleButtonEmulation()
{ 
    m_MiddleButtonEmulation = false;
    return m_MiddleButtonEmulation;
}
void WaylandMouse::setMiddleButtonEmulation(bool)
{ 

}

double WaylandMouse::MotionAcceleration()
{ 
    return m_MotionAcceleration;
}
void WaylandMouse::setMotionAcceleration(double)
{ 

}

double WaylandMouse::MotionScaling()
{ 
    return m_MotionScaling;
}
void WaylandMouse::setMotionScaling(double)
{ 

}

double WaylandMouse::MotionThreshold()
{ 
    return m_MotionThreshold;
}

void WaylandMouse::setMotionThreshold(double)
{ 

}

bool WaylandMouse::NaturalScroll()
{ 
    m_NaturalScroll = false;
    return m_NaturalScroll;
}
void WaylandMouse::setNaturalScroll(bool)
{ 

}

void WaylandMouse::Reset()
{
}