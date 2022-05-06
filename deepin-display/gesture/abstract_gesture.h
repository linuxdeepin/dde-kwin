#ifndef ABSTRACTGESTRURE_H
#define ABSTRACTGESTRURE_H

#include "../display_utils.h"

#include <QObject>

class AbstractGesture : public QObject
{
    Q_OBJECT

public:

    explicit AbstractGesture(QObject *parent = nullptr);
    ~AbstractGesture();

    virtual void SetShortPressDuration(int duration) = 0;
    virtual void SetEdgeMoveStopDuration(int duration) = 0;
    virtual void SetInputIgnore(QString node,bool isIgnore) = 0;

signals:
    void DbclickDown(int fingers);
    void Event(QString name, QString direction, int fingers);
    void SwipeMoving(int fingers, double accelX, double accelY);
    void SwipeStop(int fingers);
    void TouchEdgeEvent(QString edge, double scaleX, double scaleY);
    void TouchEdgeMoveStop(QString edge, double scaleX, double scaleY, int duration);
    void TouchEdgeMoveStopLeave(QString edge, double scaleX, double scaleY, int duration);
    void TouchMovementEvent(QString direction, int fingers, double startScaleX, double startScaleY, double endScaleX, double endScaleY);
    void TouchMoving(double scaleX, double scaleY);
    void TouchPressTimeout(int fingers, int time, double scaleX, double scaleY);
    void TouchSinglePressTimeout(int time, double scaleX, double scaleY);
    void TouchUpOrCancel(double scaleX, double scaleY);

public Q_SLOTS:

    virtual void Reset() = 0;    
};
#endif

