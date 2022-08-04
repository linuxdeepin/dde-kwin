#ifndef X11OUTPUT_H
#define X11OUTPUT_H

#include "abstract_output.h"

#include <QObject>
#include <QDBusContext>
#include <QVariant>

#include <xcb/randr.h>

class XCBEventListener;

class X11Output : public AbstractOutput
{
    Q_OBJECT

public:
    explicit X11Output(QObject *parent = nullptr);
    ~X11Output();

    virtual QVariantList Touchscreens() override;
    virtual QString PrimaryScreenName() override;
    virtual QStringList Monitors() override;
    virtual uint MaxBacklightBrightness() override;
    virtual ushort (DisplayWidth)() override;
    virtual ushort (DisplayHeight)() override;
    virtual QMap<QString, QVariant> TouchMap() override;
    virtual int ColorTemperatureMode() override;
    virtual uchar DisplayMode() override;
    virtual double Scale() override;

public:
    QString formatOutputName(QString strOutputName);
    void colorramp_fill(uint16_t *gamma_r, uint16_t *gamma_g, uint16_t *gamma_b, int size, int temperature, double dBrightness = 1);
    void setBrightness(double dBrightness);
    void testGetMode();

public slots:
    void outputChanged(xcb_randr_output_t output, xcb_randr_crtc_t crtc, xcb_randr_mode_t mode, xcb_randr_connection_t connection);
    void crtcChanged(xcb_randr_crtc_t crtc, xcb_randr_mode_t mode, xcb_randr_rotation_t rotation, const QRect &geom, xcb_timestamp_t timestamp);
    void screenChanged(xcb_randr_rotation_t rotation, const QSize &sizePx, const QSize &sizeMm);


public slots:

    virtual void SetPrimaryScreen(QString strOutputName) override;
    virtual void showCursor() override;
    virtual void hideCursor() override;
    virtual void SetDisplayMode(uchar uMode, QVariantList screenInfoList) override;
    virtual void SetColorTemperatureMode(int nValue) override;
    virtual void SetScale(double dScale) override;
    virtual void mapToOutput(int nDeviceId, QString strOutputName) override;

Q_SIGNALS:
    void outputRemoved(QMap<QString, QVariant> outputInfo);
    void outputAdded();

private:
    XCBEventListener *m_x11Helper;

    int m_nColor = 6500;
    double m_dBrightness = 1.00;
    int nDisplayMode = 0;
    int m_nCCTMode = 0;
    double m_dScale = 1.00;
    QMap<QString, QVariant> touchScreensInfo;


};
#endif // DEEPINWMFAKER_H
