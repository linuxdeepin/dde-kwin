#ifndef WAYLANDMOUSE_H
#define WAYLANDMOUSE_H

#include "abstract_mouse.h"

#include <QObject>
#include <QDBusContext>

class WaylandMouse : public AbstractMouse
{
    Q_OBJECT

public:
    explicit WaylandMouse(QObject *parent = nullptr);
    ~WaylandMouse();

    bool AdaptiveAccelProfile() override;
    bool DisableTpad() override;
    bool Exist() override;
    bool LeftHanded() override;
    bool MiddleButtonEmulation() override;
    bool NaturalScroll() override;
    double MotionAcceleration() override;
    double MotionScaling() override;
    double MotionThreshold() override;
    int DoubleClick() override;
    int DragThreshold() override;

    QString DeviceList() override;

public Q_SLOTS:

    // minin client
    void Reset() override;
};

#endif // DEEPINWMFAKER_H
