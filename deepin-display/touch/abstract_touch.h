#ifndef ABSTRACTTOUCH_H
#define ABSTRACTTOUCH_H


#include <QObject>
#include <QDBusContext>

class AbstractTouch : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.KWin.Display.Touch")

    Q_PROPERTY(bool DisableIfTypeing READ getDisableIfTypeing WRITE setDisableIfTypeing)
    Q_PROPERTY(bool EdgeScroll READ getEdgeScroll WRITE setEdgeScroll)
    Q_PROPERTY(bool Exist READ getExist)
    Q_PROPERTY(bool HorizScroll READ getHorizScroll WRITE setHorizScroll)
    Q_PROPERTY(bool LeftHanded READ getLeftHanded WRITE setLeftHanded)
    Q_PROPERTY(bool NaturalScroll READ getNaturalScroll WRITE setNaturalScroll)
    Q_PROPERTY(bool PalmDetect READ getPalmDetect WRITE setPalmDetect)
    Q_PROPERTY(bool TPadEnable READ getTPadEnable WRITE setTPadEnable)
    Q_PROPERTY(bool TapClick READ getTapClick WRITE setTapClick)
    Q_PROPERTY(bool VertScroll READ getVertScroll WRITE setVertScroll)
    Q_PROPERTY(double MotionAcceleration READ getMotionAcceleration WRITE setMotionAcceleration)
    Q_PROPERTY(double MotionScaling READ getMotionScaling WRITE setMotionScaling)
    Q_PROPERTY(double MotionThreshold READ getMotionThreshold WRITE setMotionThreshold)
    Q_PROPERTY(int DeltaScroll READ getDeltaScroll WRITE setDeltaScroll)
    Q_PROPERTY(int DoubleClick READ getDoubleClick WRITE setDoubleClick)
    Q_PROPERTY(int DragThreshold READ getDragThreshold WRITE setDragThreshold)
    Q_PROPERTY(int PalmMinWidth READ getPalmMinWidth WRITE setPalmMinWidth)
    Q_PROPERTY(int PalmMinZ READ getPalmMinZ WRITE setPalmMinZ)
    Q_PROPERTY(QString DeviceList READ getDeviceList)

public:
    explicit AbstractTouch(QObject *parent = nullptr);
    virtual ~AbstractTouch();

public:
    virtual bool getDisableIfTypeing() = 0;
    virtual void setDisableIfTypeing(bool DisableIfTypeing) = 0;
    virtual bool getEdgeScroll() = 0;
    virtual void setEdgeScroll(bool EdgeScroll) = 0;
    virtual bool getExist() = 0;
    virtual bool getHorizScroll() = 0;
    virtual void setHorizScroll(bool HorizScroll) = 0;
    virtual bool getLeftHanded() = 0;
    virtual void setLeftHanded(bool LeftHanded) = 0;
    virtual bool getNaturalScroll() = 0;
    virtual void setNaturalScroll(bool NaturalScroll) = 0;
    virtual bool getPalmDetect() = 0;
    virtual void setPalmDetect(bool PalmDetect) = 0;
    virtual bool getTPadEnable() = 0;
    virtual void setTPadEnable(bool TPadEnable) = 0;
    virtual bool getTapClick() = 0;
    virtual void setTapClick(bool TapClick) = 0;
    virtual bool getVertScroll() = 0;
    virtual void setVertScroll(bool VertScroll) = 0;
    virtual double getMotionAcceleration() = 0;
    virtual void setMotionAcceleration(double MotionAcceleration) = 0;
    virtual double getMotionScaling() = 0;
    virtual void setMotionScaling(double MotionScaling) = 0;
    virtual double getMotionThreshold() = 0;
    virtual void setMotionThreshold(double MotionThreshold) = 0;
    virtual int getDeltaScroll() = 0;
    virtual void setDeltaScroll(int DeltaScroll) = 0;
    virtual int getDoubleClick() = 0;
    virtual void setDoubleClick(int DoubleClick) = 0;
    virtual int getDragThreshold() = 0;
    virtual void setDragThreshold(int DragThreshold) = 0;
    virtual int getPalmMinWidth() = 0;
    virtual void setPalmMinWidth(int PalmMinWidth) = 0;
    virtual int getPalmMinZ() = 0;
    virtual void setPalmMinZ(int PalmMinZ)  = 0;
    virtual QString getDeviceList() = 0;

public Q_SLOTS:
    virtual void Reset() = 0;
};

#endif
