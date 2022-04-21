#ifndef ABSTRACT_INPUTDEVICES_H
#define ABSTRACT_INPUTDEVICES_H

#include "../display_utils.h"

#include <QObject>
#include <QDBusObjectPath>

class AbstractInputDevices : public QObject
{
    Q_OBJECT

public:

    ADD_PROPERTY(unsigned int, WheelSpeed);
    ADD_PROPERTY(QList<QDBusObjectPath>, Touchscreens);

    explicit AbstractInputDevices(QObject *parent = nullptr);
    ~AbstractInputDevices();
    
signals:
    void TouchscreenAdded(QDBusObjectPath);
    void TouchscreenRemoved(QDBusObjectPath);
};

#endif

