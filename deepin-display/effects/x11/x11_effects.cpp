#include "x11_effects.h"

#include <QDebug>


X11Effects::X11Effects(QObject *parent)
    : AbstractEffects(parent)
{
}

X11Effects::~X11Effects()
{
}


bool X11Effects::AllowSwitch(void) 
{
    return true;
}
QString X11Effects::CurrentWM(void)
{
    return QString();
}
void X11Effects::RequestSwitchWM(void)
{

}