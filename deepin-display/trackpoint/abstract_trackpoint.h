#ifndef ABSTRACTTRACKPOINT_H
#define ABSTRACTTRACKPOINT_H

#include "../display_utils.h"

#include <QObject>

class AbstractTrackpoint : public QObject
{
    Q_OBJECT

public:

    ADD_PROPERTY(QString, DeviceList)
    ADD_PROPERTY(bool, Exist)
    ADD_PROPERTY(bool, LeftHanded)
    ADD_PROPERTY(bool, MiddleButtonEmulation)
    ADD_PROPERTY(int, MiddleButtonTimeout)
    ADD_PROPERTY(double, MotionAcceleration)
    ADD_PROPERTY(double, MotionScaling)
    ADD_PROPERTY(double, MotionThreshold)
    ADD_PROPERTY(bool, WheelEmulation)
    ADD_PROPERTY(int, WheelEmulationButton)
    ADD_PROPERTY(int, WheelEmulationTimeout)
    ADD_PROPERTY(bool, WheelHorizScroll)

    explicit AbstractTrackpoint(QObject *parent = nullptr);
    ~AbstractTrackpoint();

public Q_SLOTS:
    virtual void Reset() = 0;

};

#endif

