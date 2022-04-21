#include "x11_mouse.h"

#include <QDebug>

X11Mouse:: X11Mouse(QObject *parent)
    : AbstractMouse(parent)
{
}

X11Mouse::~X11Mouse()
{
}


bool X11Mouse::AdaptiveAccelProfile()
{
    m_AdaptiveAccelProfile = false;
    return m_AdaptiveAccelProfile;
}

void X11Mouse::setAdaptiveAccelProfile(bool)
{ 

}
QString X11Mouse::DeviceList()
{ 
    return m_DeviceList;
}
void X11Mouse::setDeviceList(QString)
{ 

}

bool X11Mouse::DisableTpad()
{ 
    m_DisableTpad = false;
    return m_DisableTpad;
}
void X11Mouse::setDisableTpad(bool)
{ 

}

unsigned int X11Mouse::DoubleClick()
{ 
    return m_DoubleClick;
}
void X11Mouse::setDoubleClick(unsigned int)
{ 

}

int X11Mouse::DragThreshold()
{ 
    return m_DragThreshold;
}
void X11Mouse::setDragThreshold(int)
{ 

}

bool X11Mouse::Exist()
{ 
    m_Exist = false;
    return m_Exist;
}
void X11Mouse::setExist(bool)
{ 

}

bool X11Mouse::LeftHanded()
{ 
    m_LeftHanded = false;
    return m_LeftHanded;
}
void X11Mouse::setLeftHanded(bool)
{ 

}

bool X11Mouse::MiddleButtonEmulation()
{ 
    m_MiddleButtonEmulation = false;
    return m_MiddleButtonEmulation;
}
void X11Mouse::setMiddleButtonEmulation(bool)
{ 

}

double X11Mouse::MotionAcceleration()
{ 
    return m_MotionAcceleration;
}
void X11Mouse::setMotionAcceleration(double)
{ 

}

double X11Mouse::MotionScaling()
{ 
    return m_MotionScaling;
}
void X11Mouse::setMotionScaling(double)
{ 

}

double X11Mouse::MotionThreshold()
{ 
    return m_MotionThreshold;
}

void X11Mouse::setMotionThreshold(double)
{ 

}

bool X11Mouse::NaturalScroll()
{ 
    m_NaturalScroll = false;
    return m_NaturalScroll;
}
void X11Mouse::setNaturalScroll(bool)
{ 

}

void X11Mouse::Reset()
{
}