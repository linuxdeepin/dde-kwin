#ifndef X11TABLET_H
#define X11TABLET_H

#include "abstract_tablet.h"

#include <QObject>

class X11Tablet : public AbstractTablet
{
    Q_OBJECT

public:
    explicit X11Tablet(QObject *parent = nullptr);
    ~X11Tablet();

    QStringList ActionInfos();
    void setActionInfos(QStringList property);

    bool CursorMode();
    void setCursorMode(bool property);

    bool Exist();
    void setExist(bool property);

    bool ForceProportions();
    void setForceProportions(bool property);

    bool LeftHanded();
    void setLeftHanded(bool property);

    bool MouseEnterRemap();
    void setMouseEnterRemap(bool property);

    QString KeyDownAction();
    void setKeyDownAction(QString property);

    QString KeyUpAction();
    void setKeyUpAction(QString property);

    QString MapOutput();
    void setMapOutput(QString property);

    int EraserPressureSensitive();
    void setEraserPressureSensitive(int property);

    int EraserRawSample();
    void setEraserRawSample(int property);

    int EraserThreshold();
    void setEraserThreshold(int property);

    int StylusPressureSensitive();
    void setStylusPressureSensitive(int property);

    int StylusRawSample();
    void setStylusRawSample(int property);

    int StylusThreshold();
    void setStylusThreshold(int property);

    int Suppress();
    void setSuppress(int property);

    QString DeviceList();
    void setDeviceList(QString property);

public Q_SLOTS:

    // minin client
    void Reset() override;
};

#endif 
