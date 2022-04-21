#ifndef X11OUTPUT_H
#define X11OUTPUT_H

#include "abstract_output.h"

#include <QObject>
#include <QDBusContext>
#include <QVariant>

class X11Output : public AbstractOutput
{
    Q_OBJECT

public:
    explicit X11Output(QObject *parent = nullptr);
    ~X11Output();

    virtual QStringList Monitors();
    virtual QStringList CustomIdList();
    virtual QVariantList Touchscreens();
    virtual bool HasChanged();
    virtual uchar DisplayMode();
    virtual QMap<QString, QVariant> Brightness();
    virtual QMap<QString, QVariant> TouchMap();
    virtual int ColorTemperatureManual();
    virtual int ColorTemperatureMode();
    virtual QString CurrentCustomId();
    virtual QString Primary();
    virtual QVariantList PrimaryRect();
    virtual ushort ScreenHeight();
    virtual ushort ScreenWidth();
    virtual uint MaxBacklightBrightness();

public slots:
    virtual void ApplyChanged();
    virtual void AssociateTouch(QString strOutputName, QString strTouchSerial);
    virtual void AssociateTouchByUUID(QString strOutputName, QString strTouchUUID);
    virtual bool CanRotate();
    virtual bool CanSetBrightness(QString strOutputName);
    virtual void ChangeBrightness(bool bRaised);
    virtual void DeleteCustomMode(QString strName);
    virtual QMap<QString, QVariant> GetBrightness();
    virtual QStringList GetBuiltinMonitor();// ???
    virtual uchar GetRealDisplayMode();
    virtual QStringList ListOutputNames();
    virtual QList<QVariant> ListOutputsCommonModes();
    virtual void ModifyConfigName(QString strName, QString strNewName);
    virtual void RefreshBrightness();
    virtual void Reset();
    virtual void ResetChanges();
    virtual void Save();
    virtual void SetAndSaveBrightness(QString strOutputName, double dValue);
    virtual void SetBrightness(QString strOutputName, double dValue);
    virtual void SetColorTemperature(int nValue);
    virtual void SetMethodAdjustCCT(int nAdjustMethod);
    virtual void SetPrimary(QString strOutputName);
    virtual void SwitchMode(uchar uMode, QString strName);

};

#endif // DEEPINWMFAKER_H
