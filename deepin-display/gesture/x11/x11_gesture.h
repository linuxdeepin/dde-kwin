#ifndef X11GESTURE_H
#define X11GESTURE_H

#include "abstract_gesture.h"

#include <QObject>

class X11Gesture : public AbstractGesture
{
    Q_OBJECT

public:
    explicit X11Gesture(QObject *parent = nullptr);
    ~X11Gesture();

    void SetShortPressDuration(int duration);
    void SetEdgeMoveStopDuration(int duration);
    void SetInputIgnore(QString node,bool isIgnore);

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

    // minin client
    void Reset() override;
};

#endif 
