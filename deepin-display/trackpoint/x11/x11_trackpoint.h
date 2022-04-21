#ifndef X11TRACKPOINT_H
#define X11TRACKPOINT_H

#include "abstract_trackpoint.h"

#include <QObject>

class X11Trackpoint : public AbstractTrackpoint
{
    Q_OBJECT

public:
    explicit X11Trackpoint(QObject *parent = nullptr);
    ~X11Trackpoint();

    QString DeviceList() override; 
    void setDeviceList(QString) override;

    bool Exist() override; 
    void setExist(bool) override;

    bool LeftHanded() override; 
    void setLeftHanded(bool) override;

    bool MiddleButtonEmulation() override; 
    void setMiddleButtonEmulation(bool) override;

    int MiddleButtonTimeout() override; 
    void setMiddleButtonTimeout(int) override;

    double MotionAcceleration() override; 
    void setMotionAcceleration(double) override;

    double MotionScaling() override; 
    void setMotionScaling(double) override;

    double MotionThreshold() override; 
    void setMotionThreshold(double) override;

    bool WheelEmulation() override; 
    void setWheelEmulation(bool) override;

    int WheelEmulationButton() override; 
    void setWheelEmulationButton(int) override;

    int WheelEmulationTimeout() override; 
    void setWheelEmulationTimeout(int) override;
    
    bool WheelHorizScroll() override; 
    void setWheelHorizScroll(bool) override;

public Q_SLOTS:

    void Reset() override;
};

#endif 
