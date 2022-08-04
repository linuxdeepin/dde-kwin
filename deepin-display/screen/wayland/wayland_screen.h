#ifndef WaylandSCREEN_H
#define WaylandSCREEN_H

#include "abstract_screen.h"

// system
#include <unistd.h>
#include <math.h>

#include <QObject>
#include <QDBusContext>
#include <QVariant>

#include "DWayland/Client/outputdevice_v2.h"
#include "DWayland/Client/outputconfiguration_v2.h"
#include "DWayland/Client/outputmanagement_v2.h"

using namespace KWayland::Client;

class WaylandScreen : public AbstractScreen
{
    Q_OBJECT

public:
    explicit WaylandScreen(OutputDeviceV2 *pOutputDevice = nullptr, OutputManagementV2 *pOutputManagementV2 = nullptr, QObject *parent = nullptr);
    ~WaylandScreen();

public :
    virtual int ColorTemperatureManual() override;
    virtual double Brightness() override;
    virtual QMap<QString, QVariant> BestDisplayMode() override;
    virtual int ScreenWidth() override;
    virtual int ScreenHeight() override;
    virtual double RefreshRate() override;
    virtual bool Connected() override;
    virtual QString Model() override;
    virtual QVariantList Rotations() override;
    virtual int X() override;
    virtual int Y() override;
    virtual QString ID() override;
    virtual bool Enabled() override;
    virtual int PhysicalWidth() override;
    virtual int PhysicalHeight() override;
    virtual int Rotation() override;
    virtual QString Name() override;
    virtual QString Manufacturer() override;
    virtual QVariantList Modes() override;
    virtual QVariantList CurrentMode() override;
    virtual QString CurrentScalingMode() override;
    virtual QVariantList SupportScalingMode() override;

public slots:

    virtual void SetBrightness(double dBrightness) override;
    virtual void SetResolution(int nWidth, int nHeight) override;
    virtual void SetRefreshRate(double dRefreshRate) override;
    virtual void SetRotation(int nRotation) override;
    virtual void SetColorTemperature(int nColorTemperature) override;
    virtual void SetScreenPosition(int x, int y) override;
    virtual void setScalingMode(int nMode) override;

private:
    QVariantList m_modesList;
    OutputManagementV2 *m_outputManagement = nullptr;
    OutputDeviceV2 *m_pOutputDevice;
    OutputConfigurationV2 *m_outputConfiguration = nullptr;
};

#endif // WaylandSCREEN_H
