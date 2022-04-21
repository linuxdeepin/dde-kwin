#ifndef ABSTRACT_OUTPUT_H
#define ABSTRACT_OUTPUT_H

#include <QObject>
#include <QDBusContext>
#include <QMap>
#include <QVariant>
#include <QDBusObjectPath>

// kwin display dbus
#define DisplayDBusService "com.deepin.KWin.Display"
#define OutputDBusPath "/Output"

class AbstractOutput : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.KWin.Display.Output")

    Q_PROPERTY(QStringList Monitors READ Monitors)
    Q_PROPERTY(QStringList CustomIdList READ CustomIdList)
    Q_PROPERTY(QVariantList Touchscreens READ Touchscreens)
    Q_PROPERTY(bool HasChanged READ HasChanged)
    Q_PROPERTY(uchar DisplayMode READ DisplayMode)
    Q_PROPERTY(QMap Brightness READ Brightness)
    Q_PROPERTY(QMap TouchMap READ TouchMap)
    Q_PROPERTY(int ColorTemperatureManual READ ColorTemperatureManual)
    Q_PROPERTY(int ColorTemperatureMode READ ColorTemperatureMode)
    Q_PROPERTY(QString CurrentCustomId READ CurrentCustomId)
    Q_PROPERTY(QString Primary READ Primary)
    Q_PROPERTY(QVariantList PrimaryRect READ PrimaryRect)
    Q_PROPERTY(ushort ScreenHeight READ ScreenHeight)
    Q_PROPERTY(ushort ScreenWidth READ ScreenWidth)
    Q_PROPERTY(uint MaxBacklightBrightness READ MaxBacklightBrightness)

public:
    explicit AbstractOutput(QObject *parent = nullptr);
    ~AbstractOutput();

public :
    virtual QStringList Monitors() = 0;
    virtual QStringList CustomIdList() = 0;
    virtual QVariantList Touchscreens() = 0;
    virtual bool HasChanged() = 0;
    virtual uchar DisplayMode() = 0;
    virtual QMap<QString, QVariant> Brightness() = 0;
    virtual QMap<QString, QVariant> TouchMap() = 0;
    virtual int ColorTemperatureManual() = 0;
    virtual int ColorTemperatureMode() = 0;
    virtual QString CurrentCustomId() = 0;
    virtual QString Primary() = 0;
    virtual QVariantList PrimaryRect() = 0;
    virtual ushort ScreenHeight() = 0;
    virtual ushort ScreenWidth() = 0;
    virtual uint MaxBacklightBrightness() = 0;

public slots:
    virtual void ApplyChanged() = 0;
    virtual void AssociateTouch(QString strOutputName, QString strTouchSerial) = 0;
    virtual void AssociateTouchByUUID(QString strOutputName, QString strTouchUUID) = 0;
    virtual bool CanRotate() = 0;
    virtual bool CanSetBrightness(QString strOutputName) = 0;
    virtual void ChangeBrightness(bool bRaised) = 0;
    virtual void DeleteCustomMode(QString strName) = 0;
    virtual QMap<QString, QVariant> GetBrightness() = 0;
    virtual QStringList GetBuiltinMonitor() = 0;
    virtual uchar GetRealDisplayMode() = 0;
    virtual QStringList ListOutputNames() = 0;
    virtual QList<QVariant> ListOutputsCommonModes() = 0;
    virtual void ModifyConfigName(QString strName, QString strNewName) = 0;
    virtual void RefreshBrightness() = 0;
    virtual void Reset() = 0;
    virtual void ResetChanges() = 0;
    virtual void Save() = 0;
    virtual void SetAndSaveBrightness(QString strOutputName, double dValue) = 0;
    virtual void SetBrightness(QString strOutputName, double dValue) = 0;
    virtual void SetColorTemperature(int nValue) = 0;
    virtual void SetMethodAdjustCCT(int nAdjustMethod) = 0;
    virtual void SetPrimary(QString strOutputName) = 0;
    virtual void SwitchMode(uchar uMode, QString strName) = 0;

};

#endif // ABSTRACT_OUTPUT_H
