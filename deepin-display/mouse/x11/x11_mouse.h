#ifndef X11MOUSE_H
#define X11MOUSE_H

#include "abstract_mouse.h"

#include <QObject>

class X11Mouse : public AbstractMouse
{
    Q_OBJECT

public:
    explicit X11Mouse(QObject *parent = nullptr);
    ~X11Mouse();

    bool AdaptiveAccelProfile() override;
    void setAdaptiveAccelProfile(bool AdaptiveAccelProfile) override;

    QString DeviceList() override;
    void setDeviceList(QString) override;

    bool DisableTpad() override;
    void setDisableTpad(bool) override;

    unsigned int DoubleClick() override;
    void setDoubleClick(unsigned int) override;

    int DragThreshold() override;
    void setDragThreshold(int) override;

    bool Exist() override;
    void setExist(bool) override;

    bool LeftHanded() override;
    void setLeftHanded(bool) override;

    bool MiddleButtonEmulation() override;
    void setMiddleButtonEmulation(bool) override;

    double MotionAcceleration() override;
    void setMotionAcceleration(double) override;

    double MotionScaling() override;
    void setMotionScaling(double) override;

    double MotionThreshold() override;
    void setMotionThreshold(double) override;

    bool NaturalScroll() override;
    void setNaturalScroll(bool) override;



public Q_SLOTS:

    // minin client
    void Reset() override;
};

#endif 
