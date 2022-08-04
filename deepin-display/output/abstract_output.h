#ifndef ABSTRACT_OUTPUT_H
#define ABSTRACT_OUTPUT_H

#include <QObject>
#include <QDBusContext>
#include <QMap>
#include <QVariant>
#include <QDBusObjectPath>

// kwin display dbus
#define DisplayDBusService "com.deepin.KWin.Display"
#define OutputDBusPath "/Output"

typedef double	XDouble;
typedef int XFixed;

#define XDoubleToFixed(f)    ((XFixed) ((f) * 65536))
#define XFixedToDouble(f)    (((XDouble) (f)) / 65536)
#define RR_Reflect_All	(RR_Reflect_X|RR_Reflect_Y)

typedef struct Matrix {
    float m[9];
} Matrix;

class AbstractOutput : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.KWin.Display.Output")

    Q_PROPERTY(QVariantList Touchscreens READ Touchscreens)
    Q_PROPERTY(QString PrimaryScreenName READ PrimaryScreenName)
    Q_PROPERTY(QStringList Monitors READ Monitors)
    Q_PROPERTY(uint MaxBacklightBrightness READ MaxBacklightBrightness)
    Q_PROPERTY(ushort DisplayWidth READ DisplayWidth)
    Q_PROPERTY(ushort DisplayHeight READ DisplayHeight)
    Q_PROPERTY(QMap TouchMap READ TouchMap)
    Q_PROPERTY(int ColorTemperatureMode READ ColorTemperatureMode)
    Q_PROPERTY(uchar DisplayMode READ DisplayMode)
    Q_PROPERTY(double Scale READ Scale)

public:
    explicit AbstractOutput(QObject *parent = nullptr);
    ~AbstractOutput();

    enum displayMode {
        screenExtendMode = 1,
        screenCopyMode = 2,
        screenShowAloneMode = 3,
    };

    enum CCTMode {
        noMode = 0,
        autoMode = 1,
        manualmode = 2,
    };

    enum Type {
        Unknown,
        VGA,
        DVI,
        DVII,
        DVIA,
        DVID,
        HDMI,
        Panel,
        TV,
        TVComposite,
        TVSVideo,
        TVComponent,
        TVSCART,
        TVC4,
        DisplayPort,
    };
    Q_ENUM(Type)

public :
    virtual QVariantList Touchscreens() = 0;
    virtual QString PrimaryScreenName() = 0;
    virtual QStringList Monitors() = 0;
    virtual uint MaxBacklightBrightness() = 0;
    virtual ushort DisplayWidth() = 0;
    virtual ushort DisplayHeight() = 0;
    virtual QMap<QString, QVariant> TouchMap() = 0;
    virtual int ColorTemperatureMode() = 0;
    virtual uchar DisplayMode() = 0;
    virtual double Scale() = 0;

public slots:
    virtual void SetPrimaryScreen(QString strOutputName) = 0;
    virtual void showCursor() = 0;
    virtual void hideCursor() = 0;
    virtual void SetDisplayMode(uchar uMode, QVariantList screenInfoList) = 0;
    virtual void SetColorTemperatureMode(int nValue) = 0;
    virtual void SetScale(double dScale) = 0;
    virtual void mapToOutput(int nDeviceId, QString strOutputName) = 0;

Q_SIGNALS:
    void outputRemoved(QMap<QString, QVariant> outputInfo);
    void outputAdded();

};

#endif // ABSTRACT_OUTPUT_H
