#ifndef WAYLANDOUTPUT_H
#define WAYLANDOUTPUT_H

#include "abstract_output.h"

#include <QObject>
#include <QDBusContext>
#include <QVariant>
#include <QThread>
#include <QTime>
#include <QVector>

// system
#include <unistd.h>
#include <math.h>

#include "DWayland/Client/outputdevice_v2.h"
#include "DWayland/Client/connection_thread.h"
#include "DWayland/Client/clientmanagement.h"
#include "DWayland/Client/event_queue.h"
#include "DWayland/Client/registry.h"
#include "DWayland/Client/outputmanagement_v2.h"
#include "DWayland/Client/outputconfiguration_v2.h"
#include "DWayland/Client/dpms.h"
#include "DWayland/Client/seat.h"
#include "DWayland/Client/surface.h"

#include "DWayland/Server/display.h"
#include "DWayland/Server/ddeshell_interface.h"
#include "DWayland/Server/compositor_interface.h"
#include "DWayland/Server/outputconfiguration_v2_interface.h"
#include "DWayland/Server/outputdevice_v2_interface.h"
#include "DWayland/Server/outputmanagement_v2_interface.h"
#include "DWayland/Server/dpms_interface.h"
#include "DWayland/Server/seat_interface.h"

using namespace KWayland::Client;

class WaylandOutput : public AbstractOutput
{
    Q_OBJECT

public:
    explicit WaylandOutput(QObject *parent = nullptr);
    ~WaylandOutput();

    void init();

    virtual QVariantList Touchscreens() override;
    virtual QString PrimaryScreenName() override;
    virtual QStringList Monitors() override;
    virtual uint MaxBacklightBrightness() override;
    virtual ushort DisplayWidth() override;
    virtual ushort DisplayHeight() override;
    virtual QMap<QString, QVariant> TouchMap() override;
    virtual int ColorTemperatureMode() override;
    virtual uchar DisplayMode() override;
    virtual double Scale() override;

public:
    OutputDeviceV2 *findOutputDeviceForName(QString strName);
    QString findNameForOutputDevice(OutputDeviceV2 *pOutputDevice);

    Output* findOutputForName(QString strName);
    QString findNameForOutput(Output *pOutput);

    OutputDeviceV2 *findOutputDeviceForUuid(QString strUuid);

    void setDpmsStatus(Dpms::Mode mode, Output *pOutput = nullptr);

Q_SIGNALS:
    void outputRemoved(QMap<QString, QVariant> outputInfo);
    void outputAdded();

public slots:

    virtual void SetPrimaryScreen(QString strOutputName) override;
    virtual void showCursor() override;
    virtual void hideCursor() override;
    virtual void SetDisplayMode(uchar uMode, QVariantList screenInfoList) override;
    virtual void SetColorTemperatureMode(int nValue) override;
    virtual void SetScale(double dScale) override;
    virtual void mapToOutput(int nDeviceId, QString strOutputName) override;

private:
    void setupRegistry(Registry *registry);
    QThread *m_connectionThread;
    ConnectionThread *m_connectionThreadObject;
    EventQueue *m_eventQueue = nullptr;
    OutputManagementV2 *m_outputManagement = nullptr;
    QList<OutputDeviceV2 *> m_outputDeviceLst;
    OutputConfigurationV2 *m_outputConfiguration = nullptr;
    QVector<ClientManagement::WindowState> m_windowStates;

    KWaylandServer::CompositorInterface *m_compositorInterface;
    KWaylandServer::Display *m_display;
    Compositor *m_compositor;
    KWaylandServer::SeatInterface *m_pSeatInterface;
    Seat *m_pSeat;
    Surface *m_Surface;

    QStringList m_outputNameLst; //all output name , for example HDMI-A-1,VGA-1
    QString m_strPrimaryScreenName;

    DpmsManager *m_dpmsManager = nullptr;
    QList<Output *> m_OutputList;
    QMap<QString, QVariant> outputBrightnessMap;
    int nDisplayMode = 0;
    int m_nCCTMode = 0;
    int m_nCCTValue = 6500;
    double m_dScale = 1.00;

    QMap<QString, QVariant> m_associateMap;
};

#endif // DEEPINWMFAKER_H
