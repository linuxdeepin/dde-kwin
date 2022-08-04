#ifndef X11SCREEN_H
#define X11SCREEN_H

#include "abstract_screen.h"

#include <QObject>
#include <QDBusContext>
#include <QVariant>

#include <xcb/xcb.h>
#include <xcb/randr.h>


class X11Screen : public AbstractScreen
{
    Q_OBJECT

public:
    explicit X11Screen(xcb_connection_t* pXConnection, xcb_screen_t* pXFirstScreen, xcb_window_t XWindowDummy, QString &strName, xcb_randr_crtc_t xcb_randr_crtc_t, QObject *parent = nullptr);
    ~X11Screen();

public :
    int ColorTemperatureManual() override;
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

public:
    void colorrampFill(uint16_t *gamma_r, uint16_t *gamma_g, uint16_t *gamma_b, int size, int temperature, double dBrightness = 1);
    QString formatOutputName(QString strOutputName);

    void getScalingModeInfo();

    static QByteArray outputEdid(xcb_randr_output_t outputId);
    static quint8 *getXProperty(xcb_randr_output_t output, xcb_atom_t atom, size_t &len);
    static QByteArray extractEisaId(QByteArray edid);
    static QByteArray extractSerialNumber(QByteArray edid);
    static QByteArray extractMonitorName(QByteArray edid);

private:
    xcb_connection_t* m_pXConnection = nullptr;
    xcb_screen_t* m_pXFirstScreen = nullptr;
    xcb_window_t m_XWindowDummy;
    xcb_randr_crtc_t m_xcb_randr_crtc_t;

    QVariantList m_modesList;
    QString m_strScreenName;
    int m_nColor = 6500;
    double m_dBrightness = 1.00;
    QString m_strUuid;

    mutable QByteArray m_edid;
    xcb_randr_output_t m_id;
    QString m_strManufacturer;
    QString m_strSerialNumber;
    QString m_strMonitorName;

    QVariantList m_supportScalingModeList;
    QString currentScalingMode = "None";
};

#endif // X11SCREEN_H
