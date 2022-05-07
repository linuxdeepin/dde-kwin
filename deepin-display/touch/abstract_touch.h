#ifndef ABSTRACTTOUCH_H
#define ABSTRACTTOUCH_H


#include <QObject>
#include <QDBusContext>

class AbstractTouch : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.KWin.Display.Touch")

    Q_PROPERTY(bool DisableIfTypeing READ getDisableIfTypeing WRITE setDisableIfTypeing)//是否在输入时禁用
    Q_PROPERTY(bool EdgeScroll READ getEdgeScroll WRITE setEdgeScroll)//是否使用边缘滚动
    Q_PROPERTY(bool Exist READ getExist)//是否存在
    Q_PROPERTY(bool HorizScroll READ getHorizScroll WRITE setHorizScroll)//是否使用水平滚动
    Q_PROPERTY(bool LeftHanded READ getLeftHanded WRITE setLeftHanded)//是否使用左手模式
    Q_PROPERTY(bool NaturalScroll READ getNaturalScroll WRITE setNaturalScroll)//是否使用自然滚动
    Q_PROPERTY(bool PalmDetect READ getPalmDetect WRITE setPalmDetect)//是否开启手掌误触检测
    Q_PROPERTY(bool TPadEnable READ getTPadEnable WRITE setTPadEnable)//是否开启
    Q_PROPERTY(bool TapClick READ getTapClick WRITE setTapClick)//是否点击
    Q_PROPERTY(bool VertScroll READ getVertScroll WRITE setVertScroll)//是否使用竖直滚动
    Q_PROPERTY(double MotionAcceleration READ getMotionAcceleration WRITE setMotionAcceleration)//移动加速
    Q_PROPERTY(double MotionScaling READ getMotionScaling WRITE setMotionScaling)//移动速度范围
    Q_PROPERTY(double MotionThreshold READ getMotionThreshold WRITE setMotionThreshold)//移动阈值
    Q_PROPERTY(int DeltaScroll READ getDeltaScroll WRITE setDeltaScroll)//滚动变化量
    Q_PROPERTY(int DoubleClick READ getDoubleClick WRITE setDoubleClick)//双击速度
    Q_PROPERTY(int DragThreshold READ getDragThreshold WRITE setDragThreshold)//拖动阈值
    Q_PROPERTY(int PalmMinWidth READ getPalmMinWidth WRITE setPalmMinWidth)//手掌误触最小宽度
    Q_PROPERTY(int PalmMinZ READ getPalmMinZ WRITE setPalmMinZ)//手掌误触最低Z轴压力
    Q_PROPERTY(QString DeviceList READ getDeviceList)//设备列表

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
