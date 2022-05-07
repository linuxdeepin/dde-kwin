#include "x11_screen.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>
#include <QDebug>
#include <QDBusReply>

X11Screen::X11Screen(QObject *parent)
    : AbstractScreen(parent)
{

}

X11Screen::~ X11Screen()
{

}

QVariantList X11Screen::Modes()
{
    return QVariantList();
}

QVariantList X11Screen::PreferredModes()
{
    return QVariantList();
}

QVariantList X11Screen::Reflects()
{
    return QVariantList();
}

QVariantList X11Screen::Rotations()
{
    return QVariantList();
}

bool X11Screen::Connected()
{
    return true;
}

bool X11Screen::Enabled()
{
    return true;
}

uchar X11Screen::CurrentRotateMode()
{
    return uchar();
}

double X11Screen::Brightness()
{
    return double();
}

double X11Screen::RefreshRate()
{
    return double();
}

int X11Screen::X()
{
    return 0;
}

int X11Screen::Y()
{
    return 0;
}

QString X11Screen::Manufacturer()
{
    return QString();
}

QString X11Screen::Model()
{
    return QString();
}

QString X11Screen::Name()
{
    return QString();
}

QVariantList X11Screen::BestMode()
{
    return QVariantList();
}

QVariantList X11Screen::CurrentMode()
{
    return QVariantList();
}

int X11Screen::Height()
{
    return 0;
}

int X11Screen::Reflect()
{
    return 0;
}

int X11Screen::Rotation()
{
    return 0;
}

int X11Screen::Width()
{
    return 0;
}

int X11Screen::ID()
{
    return 0;
}

int X11Screen::MmHeight()
{
    return 0;
}

int X11Screen::MmWidth()
{
    return 0;
}


void X11Screen::Enable(bool isEnable)
{

}

void X11Screen::SetMode(int nMode)
{

}

void X11Screen::SetModeBySize(int nWidth, int nHeight)
{

}

void X11Screen::SetPosition(int x,int y)
{

}

void X11Screen::SetReflect(int nReflect)
{

}

void X11Screen::SetRefreshRate(double dRefreshRate)
{

}

void X11Screen::SetRotation(int nRotation)
{

}
