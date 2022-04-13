#ifndef X11MOUSE_H
#define X11MOUSE_H

#include "abstract_mouse.h"

#include <QObject>
#include <QDBusContext>

class X11Mouse : public AbstractMouse
{
    Q_OBJECT

public:
    explicit X11Mouse(QObject *parent = nullptr);
    ~X11Mouse();

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
