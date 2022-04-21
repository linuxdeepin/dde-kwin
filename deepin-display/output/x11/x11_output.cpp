#include "x11_output.h"

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

X11Output::X11Output(QObject *parent)
    : AbstractOutput(parent)
{

}

X11Output::~ X11Output()
{

}

QStringList X11Output::Monitors()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QStringList();
}

QStringList X11Output::CustomIdList()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QStringList();
}

QVariantList X11Output::Touchscreens()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QVariantList();
}

bool X11Output::HasChanged()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return true;
}

uchar X11Output::DisplayMode()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return uchar();
}

QMap<QString, QVariant> X11Output::Brightness()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QMap<QString, QVariant>();
}

QMap<QString, QVariant> X11Output::TouchMap()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QMap<QString, QVariant>();
}

int X11Output::ColorTemperatureManual()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return 0;
}

int X11Output::ColorTemperatureMode()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return 0;
}

QString X11Output::CurrentCustomId()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QString();
}

QString X11Output::Primary()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QString();
}

QVariantList X11Output::PrimaryRect() // ???
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QVariantList();
}

ushort X11Output::ScreenHeight()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return ushort();
}

ushort X11Output::ScreenWidth()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return ushort();
}

uint X11Output::MaxBacklightBrightness()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return uint();
}

void X11Output::ApplyChanged()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::AssociateTouch(QString strOutputName, QString strTouchSerial)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::AssociateTouchByUUID(QString strOutputName, QString strTouchUUID)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

bool X11Output::CanRotate()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return true;
}

bool X11Output::CanSetBrightness(QString strOutputName)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return true;
}

void X11Output::ChangeBrightness(bool bRaised)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::DeleteCustomMode(QString strName)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

QMap<QString, QVariant> X11Output::GetBrightness()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QMap<QString, QVariant>();
}

QStringList X11Output::GetBuiltinMonitor()// ???
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QStringList();
}

uchar X11Output::GetRealDisplayMode()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return uchar();
}

QStringList X11Output::ListOutputNames()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QStringList();
}

QList<QVariant> X11Output::ListOutputsCommonModes()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
    return QList<QVariant>();
}

void X11Output::ModifyConfigName(QString strName, QString strNewName)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::RefreshBrightness()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::Reset()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::ResetChanges()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::Save()
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::SetAndSaveBrightness(QString strOutputName, double dValue)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::SetBrightness(QString strOutputName, double dValue)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::SetColorTemperature(int nValue)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::SetMethodAdjustCCT(int nAdjustMethod)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::SetPrimary(QString strOutputName)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}

void X11Output::SwitchMode(uchar uMode, QString strName)
{
    qDebug() << __FILE__ << __LINE__ << __FUNCTION__;
}
