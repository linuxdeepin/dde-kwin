#include "x11_keyboard.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>
#include <QMap>

X11Keyboard:: X11Keyboard(QObject *parent)
    : AbstractKeyboard(parent)
{
}

X11Keyboard::~ X11Keyboard()
{
}

bool X11Keyboard::CapslockToggle()
{ 
    m_CapslockToggle = false;
    return m_CapslockToggle;
}

void X11Keyboard::setCapslockToggle(bool)
{  

}

QString X11Keyboard::CurrentLayout()
{ 
    return m_CurrentLayout;
}

void X11Keyboard::setCurrentLayout(QString)
{  

}

int X11Keyboard::CursorBlink()
{  
    return m_CursorBlink;
}

void X11Keyboard::setCursorBlink(int)
{  

}

int X11Keyboard::LayoutScope()
{  
    return m_LayoutScope;
}

void X11Keyboard::setLayoutScope(int)
{  

}
int X11Keyboard::RepeatDelay()
{  
    return m_RepeatDelay;
}

void X11Keyboard::setRepeatDelay(int)
{  

}

bool X11Keyboard::RepeatEnabled()
{
    m_RepeatEnabled = false;
    return m_RepeatEnabled;
}

void X11Keyboard::setRepeatEnabled(bool)
{

}

unsigned int X11Keyboard::RepeatInterval()
{  
    return m_RepeatInterval;
}

void X11Keyboard::setRepeatInterval(unsigned int)
{  

}

QList<QString> X11Keyboard::UserLayoutList()
{  
    return m_UserLayoutList;
}

void X11Keyboard::setUserLayoutList(QList<QString>)
{  

}

QList<QString> X11Keyboard::UserOptionList()
{  
    return m_UserOptionList;
}

void X11Keyboard::setUserOptionList(QList<QString>)
{  

}

void X11Keyboard::AddLayoutOption(QString)
{ 

}
void X11Keyboard::AddUserLayout(QString)
{ 

}
void X11Keyboard::ClearLayoutOption()
{ 

}
void X11Keyboard::DeleteLayoutOption(QString)
{ 

}
void X11Keyboard::DeleteUserLayout(QString)
{ 

}
QString X11Keyboard::GetLayoutDesc(QString)
{
    return QString();

}
QMap<QString,QString> X11Keyboard::LayoutList()
{ 
    QMap<QString,QString> res;
    return res;
}
void X11Keyboard::Reset()
{ 

}