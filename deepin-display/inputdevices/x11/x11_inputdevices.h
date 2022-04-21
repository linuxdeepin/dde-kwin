#ifndef X11_INPUTDEVICES_H
#define X11_INPUTDEVICES_H

#include "abstract_inputdevices.h"

#include <QObject>

class X11InputDevices : public AbstractInputDevices
{
    Q_OBJECT

public:
    explicit X11InputDevices(QObject *parent = nullptr);
    ~X11InputDevices();
    unsigned int WheelSpeed() override;
    void setWheelSpeed(unsigned int) override;
    QList<QDBusObjectPath> Touchscreens() override; 
    void setTouchscreens(QList<QDBusObjectPath>) override;
};

#endif 
