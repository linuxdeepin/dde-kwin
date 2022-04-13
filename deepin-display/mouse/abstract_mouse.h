#ifndef ABSTRACTMOUSE_H
#define ABSTRACTMOUSE_H


#include <QObject>
#include <QDBusContext>

// kwin display dbus
#define DisplayDBusService "com.deepin.KWin.Display"
#define MouseDBusPath "/Mouse"

class AbstractMouse : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.KWin.Display.Mouse")
    Q_PROPERTY(bool Exist READ Exist)
    Q_PROPERTY(QString DeviceList READ DeviceList)
/*
    Q_PROPERTY(bool compositingEnabled READ compositingEnabled WRITE setCompositingEnabled NOTIFY compositingEnabledChanged)
    Q_PROPERTY(bool compositingAllowSwitch READ compositingAllowSwitch FINAL)
    Q_PROPERTY(bool compositingPossible READ compositingPossible)
    Q_PROPERTY(bool zoneEnabled READ zoneEnabled WRITE setZoneEnabled)
    Q_PROPERTY(QString cursorTheme READ cursorTheme WRITE setCursorTheme)
    Q_PROPERTY(int cursorSize READ cursorSize WRITE setCursorSize)*/

public:
    explicit AbstractMouse(QObject *parent = nullptr);
    ~AbstractMouse();

    virtual bool AdaptiveAccelProfile()=0;
    virtual bool DisableTpad()=0;
    virtual bool Exist()=0;
    virtual bool LeftHanded()=0;
    virtual bool MiddleButtonEmulation()=0;
    virtual bool NaturalScroll()=0;
    virtual double MotionAcceleration()=0;
    virtual double MotionScaling()=0;
    virtual double MotionThreshold()=0;
    virtual int DoubleClick()=0;
    virtual int DragThreshold()=0;

    virtual QString DeviceList()=0;

public Q_SLOTS:

    // minin client
    Q_SCRIPTABLE virtual void Reset()=0;
};

#endif // DEEPINWMFAKER_H
