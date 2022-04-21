#include "wayland_effects.h"

#include <QDebug>

WaylandEffects::WaylandEffects(QObject *parent)
    : AbstractEffects(parent)
{
}

WaylandEffects::~WaylandEffects()
{

}
bool WaylandEffects::AllowSwitch(void) 
{
    return false;
}

QString WaylandEffects::CurrentWM(void)
{
    return QString();
}

void WaylandEffects::RequestSwitchWM(void)
{

}