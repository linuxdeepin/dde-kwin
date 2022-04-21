#include "wayland_keyboard.h"

#include <QDebug>
#include <QMap>

WaylandKeyboard::WaylandKeyboard(QObject *parent)
    : AbstractKeyboard(parent)
{
}

WaylandKeyboard::~WaylandKeyboard()
{
}

bool WaylandKeyboard::CapslockToggle()
{ 
    qDebug()<<">>>>>> get CapslockToggle";
    m_CapslockToggle = false;
    return m_CapslockToggle;
}

void WaylandKeyboard::setCapslockToggle(bool)
{  

}

QString WaylandKeyboard::CurrentLayout()
{ 
    return m_CurrentLayout;
}

void WaylandKeyboard::setCurrentLayout(QString)
{  

}

int WaylandKeyboard::CursorBlink()
{
    qDebug()<<">>>>>> get CursorBlink"; 
    return m_CursorBlink;
}

void WaylandKeyboard::setCursorBlink(int)
{  

}

int WaylandKeyboard::LayoutScope()
{  
    return m_LayoutScope;
}

void WaylandKeyboard::setLayoutScope(int)
{  

}
int WaylandKeyboard::RepeatDelay()
{  
    return m_RepeatDelay;
}

void WaylandKeyboard::setRepeatDelay(int)
{  

}

bool WaylandKeyboard::RepeatEnabled()
{
    m_RepeatEnabled = false;
    return m_RepeatEnabled;
}

void WaylandKeyboard::setRepeatEnabled(bool)
{

}

unsigned int WaylandKeyboard::RepeatInterval()
{  
    return m_RepeatInterval;
}

void WaylandKeyboard::setRepeatInterval(unsigned int)
{  

}

QList<QString> WaylandKeyboard::UserLayoutList()
{  
    return m_UserLayoutList;
}

void WaylandKeyboard::setUserLayoutList(QList<QString>)
{  

}

QList<QString> WaylandKeyboard::UserOptionList()
{  
    return m_UserOptionList;
}

void WaylandKeyboard::setUserOptionList(QList<QString>)
{  

}

void WaylandKeyboard::AddLayoutOption(QString)
{ 

}
void WaylandKeyboard::AddUserLayout(QString)
{ 

}
void WaylandKeyboard::ClearLayoutOption()
{ 

}
void WaylandKeyboard::DeleteLayoutOption(QString)
{ 

}
void WaylandKeyboard::DeleteUserLayout(QString)
{ 

}
QString WaylandKeyboard::GetLayoutDesc(QString)
{
    return QString();

}
QMap<QString,QString> WaylandKeyboard::LayoutList()
{ 
    QMap<QString,QString> res;
    return res;
}
void WaylandKeyboard::Reset()
{ 

}