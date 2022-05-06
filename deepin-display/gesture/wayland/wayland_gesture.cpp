#include "wayland_gesture.h"

#include <QDebug>

WaylandGesture::WaylandGesture(QObject *parent)
    : AbstractGesture(parent)
{
}

WaylandGesture::~WaylandGesture()
{
}

void WaylandGesture::SetShortPressDuration(int duration)
{

}

void WaylandGesture::SetEdgeMoveStopDuration(int duration)
{

}

void WaylandGesture::SetInputIgnore(QString node, bool isIgnore)
{

}

void WaylandGesture::Reset()
{
}
