#ifndef WAYLANDTOUCH_H
#define WAYLANDTOUCH_H

#include "abstract_touch.h"

#include <QObject>
#include <QDBusContext>

class WaylandTouch : public AbstractTouch
{
    Q_OBJECT

public:
    explicit WaylandTouch(QObject *parent = nullptr);
    ~WaylandTouch();

public:
    bool getDisableIfTypeing() override;
    void setDisableIfTypeing(bool DisableIfTypeing) override;
    bool getEdgeScroll() override;
    void setEdgeScroll(bool EdgeScroll) override;
    bool getExist() override;
    bool getHorizScroll() override;
    void setHorizScroll(bool HorizScroll) override;
    bool getLeftHanded() override;
    void setLeftHanded(bool LeftHanded) override;
    bool getNaturalScroll() override;
    void setNaturalScroll(bool NaturalScroll) override;
    bool getPalmDetect() override;
    void setPalmDetect(bool PalmDetect) override;
    bool getTPadEnable() override;
    void setTPadEnable(bool TPadEnable) override;
    bool getTapClick() override;
    void setTapClick(bool TapClick) override;
    bool getVertScroll() override;
    void setVertScroll(bool VertScroll) override;
    double getMotionAcceleration() override;
    void setMotionAcceleration(double MotionAcceleration) override;
    double getMotionScaling() override;
    void setMotionScaling(double MotionScaling) override;
    double getMotionThreshold() override;
    void setMotionThreshold(double MotionThreshold) override;
    int getDeltaScroll() override;
    void setDeltaScroll(int DeltaScroll) override;
    int getDoubleClick() override;
    void setDoubleClick(int DoubleClick) override;
    int getDragThreshold() override;
    void setDragThreshold(int DragThreshold) override;
    int getPalmMinWidth() override;
    void setPalmMinWidth(int PalmMinWidth) override;
    int getPalmMinZ() override;
    void setPalmMinZ(int PalmMinZ) override;
    QString getDeviceList() override;
public Q_SLOTS:
    void Reset() override;
};

#endif
