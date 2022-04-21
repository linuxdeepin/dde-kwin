#ifndef ABSTRACTMOUSE_H
#define ABSTRACTMOUSE_H

#include "../display_utils.h"

#include <QObject>

class AbstractMouse : public QObject
{
    Q_OBJECT

public:

    ADD_PROPERTY(bool, AdaptiveAccelProfile)
    ADD_PROPERTY(QString, DeviceList)
    ADD_PROPERTY(bool, DisableTpad)
    ADD_PROPERTY(unsigned int, DoubleClick)
    ADD_PROPERTY(int, DragThreshold)
    ADD_PROPERTY(bool, Exist)
    ADD_PROPERTY(bool, LeftHanded)
    ADD_PROPERTY(bool, MiddleButtonEmulation)
    ADD_PROPERTY(double, MotionAcceleration)
    ADD_PROPERTY(double, MotionScaling)
    ADD_PROPERTY(double, MotionThreshold)
    ADD_PROPERTY(bool, NaturalScroll)

    explicit AbstractMouse(QObject *parent = nullptr);
    ~AbstractMouse();

public Q_SLOTS:

    virtual void Reset() = 0;    
};
#endif

