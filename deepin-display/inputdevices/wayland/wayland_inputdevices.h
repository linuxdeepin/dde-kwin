#ifndef WAYLAND_INPUTDEVICES_H
#define WAYLAND_INPUTDEVICES_H

#include "abstract_inputdevices.h"

#include <QObject>

class WaylandInputDevices : public AbstractInputDevices
{
    Q_OBJECT

public:
    explicit WaylandInputDevices(QObject *parent = nullptr);
    ~WaylandInputDevices();

    unsigned int WheelSpeed() override;
    void setWheelSpeed(unsigned int) override;
    QList<QDBusObjectPath> Touchscreens() override; 
    void setTouchscreens(QList<QDBusObjectPath>) override;
};

#endif 
