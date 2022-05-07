#ifndef ABSTRACT_SCREEN_H
#define ABSTRACT_SCREEN_H

#include <QObject>
#include <QDBusContext>
#include <QMap>
#include <QVariant>
#include <QDBusObjectPath>

class AbstractScreen : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.KWin.Display.Screen")

    Q_PROPERTY(QVariantList Modes READ Modes)
    Q_PROPERTY(QVariantList PreferredModes READ PreferredModes)
    Q_PROPERTY(QVariantList Reflects READ Reflects)
    Q_PROPERTY(QVariantList Rotations READ Rotations)
    Q_PROPERTY(bool Connected READ Connected)
    Q_PROPERTY(bool Enabled READ Enabled)
    Q_PROPERTY(uchar CurrentRotateMode READ CurrentRotateMode)
    Q_PROPERTY(double Brightness READ Brightness)
    Q_PROPERTY(double RefreshRate READ RefreshRate)
    Q_PROPERTY(int X READ X)
    Q_PROPERTY(int Y READ Y)
    Q_PROPERTY(QString Manufacturer READ Manufacturer)
    Q_PROPERTY(QString Model READ Model)
    Q_PROPERTY(QString Name READ Name)
    Q_PROPERTY(QVariantList BestMode READ BestMode)
    Q_PROPERTY(QVariantList CurrentMode READ CurrentMode)
    Q_PROPERTY(int Height READ Height)
    Q_PROPERTY(int Reflect READ Reflect)
    Q_PROPERTY(int Rotation READ Rotation)
    Q_PROPERTY(int Width READ Width)
    Q_PROPERTY(int ID READ ID)
    Q_PROPERTY(int MmHeight READ MmHeight)
    Q_PROPERTY(int MmWidth READ MmWidth)

public:
    explicit AbstractScreen(QObject *parent = nullptr);
    ~AbstractScreen();

public :
    virtual QVariantList Modes() = 0;
    virtual QVariantList PreferredModes() = 0;
    virtual QVariantList Reflects() = 0;
    virtual QVariantList Rotations() = 0;
    virtual bool Connected() = 0;
    virtual bool Enabled() = 0;
    virtual uchar CurrentRotateMode() = 0;
    virtual double Brightness() = 0;
    virtual double RefreshRate() = 0;
    virtual int X() = 0;
    virtual int Y() = 0;
    virtual QString Manufacturer() = 0;
    virtual QString Model() = 0;
    virtual QString Name() = 0;
    virtual QVariantList BestMode() = 0;
    virtual QVariantList CurrentMode() = 0;
    virtual int Height() = 0;
    virtual int Reflect() = 0;
    virtual int Rotation() = 0;
    virtual int Width() = 0;
    virtual int ID() = 0;
    virtual int MmHeight() = 0;
    virtual int MmWidth() = 0;

public slots:
    virtual void Enable(bool isEnable) = 0;
    virtual void SetMode(int nMode) = 0;
    virtual void SetModeBySize(int nWidth, int nHeight) = 0;
    virtual void SetPosition(int x,int y) = 0;
    virtual void SetReflect(int nReflect) = 0;
    virtual void SetRefreshRate(double dRefreshRate) = 0;
    virtual void SetRotation(int nRotation) = 0;
};

#endif // ABSTRACT_SCREEN_H
