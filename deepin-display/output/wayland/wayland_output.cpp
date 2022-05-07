#include "wayland_output.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>
#include <QDBusReply>
#include <QJsonParseError>
#include <QDir>

#define DBUS_DEEPIN_WM_SERVICE "org.kde.KWin"
#define DBUS_DEEPIN_WM_OBJ "/KWin"
#define DBUS_DEEPIN_WM_INTF "org.kde.KWin"

#define DBUS_DEEPIN_DOCK_SERVICE "com.deepin.dde.daemon.Dock"
#define DBUS_DEEPIN_DOCK_OBJ "/com/deepin/dde/daemon/Dock"
#define DBUS_DEEPIN_DOCK_INTF "com.deepin.dde.daemon.Dock"

#define BLACK_LIGHT_PATH "/sys/class/backlight"

WaylandOutput:: WaylandOutput(QObject *parent)
    : AbstractOutput(parent)
{
    m_connectionThread = new QThread(this);
    m_connectionThreadObject = new ConnectionThread();

    init();
}

WaylandOutput::~ WaylandOutput()
{
    m_connectionThread->quit();
    m_connectionThread->wait();
    m_connectionThreadObject->deleteLater();
}

void WaylandOutput::init()
{
    connect(m_connectionThreadObject, &ConnectionThread::connected, this,
        [this] {
            m_eventQueue = new EventQueue(this);
            m_eventQueue->setup(m_connectionThreadObject);

            Registry *registry = new Registry(this);
            setupRegistry(registry);
        },
        Qt::QueuedConnection
    );
    m_connectionThreadObject->moveToThread(m_connectionThread);
    m_connectionThread->start();

    m_connectionThreadObject->initConnection();
}

void WaylandOutput::setupRegistry(Registry *registry)
{
    connect(registry, &Registry::outputManagementAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_outputManagement = registry->createOutputManagement(name, version, this);
            m_outputManagement->setEventQueue(m_eventQueue);
        }
    );

    connect(registry, &Registry::outputDeviceAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            OutputDevice *pOutputDevice = registry->createOutputDevice(name, version, this);
            if (pOutputDevice) {
                m_outputDeviceLst << pOutputDevice;
                connect(pOutputDevice, &OutputDevice::changed, this, [=](){

                });
            }
        }
    );

    connect(registry, &Registry::dpmsAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_dpmsManager = registry->createDpmsManager(name, version, this);
            m_dpmsManager->setEventQueue(m_eventQueue);
        }
    );

    connect(registry, &Registry::outputAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            Output *pOutput = registry->createOutput(name, version, this);
            if (pOutput) {
                m_OutputList << pOutput;
                connect(pOutput, &Output::changed, this, [=](){

                });
            }
        }
    );

    registry->setEventQueue(m_eventQueue);
    registry->create(m_connectionThreadObject);
    registry->setup();
}

QStringList WaylandOutput::Monitors()
{
    return QStringList();
}

// Abandonment method
QStringList WaylandOutput::CustomIdList()
{
    return QStringList();
}

QVariantList WaylandOutput::Touchscreens()
{
    return QVariantList();
}

// about configuration file
bool WaylandOutput::HasChanged()
{
    return true;
}

// check taskbar display mode
uchar WaylandOutput::DisplayMode()
{
    int nDockMode = 0;
    QDBusInterface dockInter(DBUS_DEEPIN_DOCK_SERVICE,
                             DBUS_DEEPIN_DOCK_OBJ,
                             DBUS_DEEPIN_DOCK_INTF,
                             QDBusConnection::sessionBus());
    if (dockInter.isValid()) {
        nDockMode = dockInter.property("DisplayMode").toInt();
    }
    return nDockMode;
}

QMap<QString, QVariant> WaylandOutput::Brightness()
{
    if (outputBrightnessMap.count() == 0) {
        QMap<QString, QVariant> brightnessInfo;
        for (int i = 0; i < m_outputDeviceLst.count(); i++) {
            OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
            QString strScreenName = findNameForOutputDevice(pOutputDevice);
            if (strScreenName != "") {
                brightnessInfo[strScreenName] = 1;
            }

        }
        return brightnessInfo;
    }

    return outputBrightnessMap;
}

QMap<QString, QVariant> WaylandOutput::TouchMap()
{
    return m_associateMap;
}

int WaylandOutput::ColorTemperatureManual()
{
    return  m_nCCTValue;
}

int WaylandOutput::ColorTemperatureMode()
{
    return m_strCCTMode;
}

// Abandonment method
QString WaylandOutput::CurrentCustomId()
{
    return QString();
}

QString WaylandOutput::Primary()
{
    return m_strPrimaryScreenName;
}

QVariantList WaylandOutput::PrimaryRect()
{
    QVariantList primaryRectList;
    OutputDevice *pOutputDevice = findOutputDeviceForName(m_strPrimaryScreenName);
    if (pOutputDevice != nullptr) {
        QRect outputDeviceRect = pOutputDevice->geometry();
        primaryRectList << outputDeviceRect.x() << outputDeviceRect.y() << outputDeviceRect.width() << outputDeviceRect.height();
    }

    return primaryRectList;
}

ushort WaylandOutput::ScreenHeight()
{
    int displayheight = 0;
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
        if (pOutputDevice->geometry().y() + pOutputDevice->geometry().height() > displayheight) {
            displayheight = pOutputDevice->geometry().y() + pOutputDevice->geometry().height();
        }
    }
    return displayheight;
}

ushort WaylandOutput::ScreenWidth()
{
    int displayWidth = 0;
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
        if (pOutputDevice->geometry().x() + pOutputDevice->geometry().width() > displayWidth) {
            displayWidth = pOutputDevice->geometry().x() + pOutputDevice->geometry().width();
        }
    }
    return displayWidth;
}

uint WaylandOutput::MaxBacklightBrightness()
{
    QDir dir(BLACK_LIGHT_PATH);
    if (!dir.exists()) {
        return uint();
    }

    QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < names.count(); i++) {
        QString str = QString(BLACK_LIGHT_PATH).append(names.at(i)).append("/max_brightness");
        QFile file(str);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return uint();
        }
        while (!file.atEnd())
        {
            QByteArray line = file.readLine();
            QString str = QString::fromStdString(line.toStdString());
            if (str != "") {
                return  str.remove(QChar('\n'), Qt::CaseInsensitive).toUInt();
            }
        }
    }
    return uint();
}

// about configuration file
void WaylandOutput::ApplyChanged()
{

}

void WaylandOutput::AssociateTouch(QString strOutputName, QString strTouchSerial)
{
    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);
    QDBusReply<QString> getReply = wm.call("getTouchDeviceToScreenInfo");
    if(!getReply.value().isEmpty()) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(getReply.value().toUtf8(), &error);
        if(QJsonParseError::NoError == error.error) {
            QList<QVariant> list = document.toVariant().toList();
            foreach(QVariant item, list)
            {
                QVariantMap map = item.toMap();
                if (findOutputDeviceForName(strOutputName) == nullptr) {
                    sendErrorReply(QDBusError::ErrorType::Failed, "No such as touch screen exists");
                    return;
                }
                QString strUUid = QString::fromStdString(findOutputDeviceForName(strOutputName)->uuid().toStdString());
                if (strUUid.contains(strTouchSerial)) {
                    for (int i = 0; i < map.keys().count(); i++) {
                        if (map.keys().at(i) == "TouchDevice") {
                            m_associateMap[strOutputName] = strTouchSerial;
                        }
                    }
                }
            }
        }
    }
}

void WaylandOutput::AssociateTouchByUUID(QString strOutputName, QString strTouchUUID)
{
    AssociateTouch(strOutputName, strTouchUUID);
}

bool WaylandOutput::CanRotate()
{
    QString strRotate = QProcessEnvironment::systemEnvironment().value("DEEPIN_DISPLAY_DISABLE_ROTATE");
    if (strRotate == "1") {
        return true;
    }
    return false;
}

bool WaylandOutput::CanSetBrightness(QString strOutputName)
{
    QString strBrightness = QProcessEnvironment::systemEnvironment().value("CAN_SET_BRIGHTNESS");
    if (strBrightness == "N" && findOutputDeviceForName(strOutputName)->model().startsWith("HDMI")) {
        return false;
    }
    return true;
}

void WaylandOutput::ChangeBrightness(bool bRaised)
{
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
        double dBrightness = outputBrightnessMap[findNameForOutputDevice(pOutputDevice)].toDouble();
        if (bRaised) {
            dBrightness += 0.1;
            if (dBrightness > 1) {
                dBrightness = 1;
            }
        } else {
            dBrightness -= 0.1;
            if (dBrightness < 0.1) {
                dBrightness = 0.1;
            }
        }
        SetBrightness(findNameForOutputDevice(pOutputDevice), dBrightness);
    }
}

// Abandonment method
void WaylandOutput::DeleteCustomMode(QString strName)
{

}

QMap<QString, QVariant> WaylandOutput::GetBrightness()
{
    if (outputBrightnessMap.count() == 0) {
        QMap<QString, QVariant> brightnessMap;
        for (int i = 0; i < m_outputDeviceLst.count(); i++) {
            OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);

            qDebug() << pOutputDevice->model() << pOutputDevice->eisaId() << pOutputDevice->serialNumber();

            brightnessMap[findNameForOutputDevice(pOutputDevice)] = 1;
        }
        return brightnessMap;
    }

    return  outputBrightnessMap;
}

QStringList WaylandOutput::GetBuiltinMonitor()
{
    QStringList strBuiltinMonitorList;
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
        if (pOutputDevice->model().startsWith("edp")) {
            strBuiltinMonitorList << findNameForOutputDevice(pOutputDevice);
        }
    }
    return strBuiltinMonitorList;
}

uchar WaylandOutput::GetRealDisplayMode()
{
    if (nDisplayMode == 0) {
        if (m_outputDeviceLst.count() > 1) {
            for (int i = 0; i < m_outputDeviceLst.count(); i++) {
                OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
                if (pOutputDevice->geometry().x() > 0 || pOutputDevice->geometry().y() > 0) {
                    return screenExtendMode;
                }
            }
            return screenCopyMode;
        } else {
            return screenShowAloneMode;
        }
    }

    return nDisplayMode;
}

QStringList WaylandOutput::ListOutputNames()
{
    QStringList strOutputNamelist;
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
        strOutputNamelist << findNameForOutputDevice(pOutputDevice);
    }
    return strOutputNamelist;
}

// Abandonment method
QList<QVariant> WaylandOutput::ListOutputsCommonModes()
{
    return QList<QVariant>();
}

// Abandonment method
void WaylandOutput::ModifyConfigName(QString strName, QString strNewName)
{

}

// RefreshBrightness Reset brightness,be call by session/power,Restore brightness from configuration
void WaylandOutput::RefreshBrightness()
{
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
        SetBrightness(findNameForOutputDevice(pOutputDevice), 1.00);
    }
}

void WaylandOutput::Reset()
{

}

void WaylandOutput::ResetChanges()
{

}

void WaylandOutput::Save()
{

}

void WaylandOutput::SetAndSaveBrightness(QString strOutputName, double dValue)
{
    SetBrightness(strOutputName, dValue);
}

void WaylandOutput::SetBrightness(QString strOutputName, double dValue)
{
    OutputDevice *pOutputDevice = findOutputDeviceForName(strOutputName);
    OutputDevice::ColorCurves colorCurves = pOutputDevice->colorCurves();

    m_outputConfiguration = m_outputManagement->createConfiguration(this);
    int rampsize = colorCurves.red.size();

    float gammaRed = 1.0;
    float gammaGreen = 1.0;
    float gammaBlue = 1.0;
    QVector<quint16> vecRed;
    QVector<quint16> vecGreen;
    QVector<quint16> vecBlue;

    for (int i = 0; i < rampsize; i++) {
        if (gammaRed == 1.0 && dValue == 1.0)
            vecRed.append((double)i / (double)(rampsize - 1) * (double)UINT16_MAX);
        else
            vecRed.append(dmin(pow((double)i/(double)(rampsize - 1),
                                   gammaRed) * dValue,
                               1.0) * (double)UINT16_MAX);

        if (gammaGreen == 1.0 && dValue == 1.0)
            vecGreen.append((double)i / (double)(rampsize - 1) * (double)UINT16_MAX);
        else
            vecGreen.append(dmin(pow((double)i/(double)(rampsize - 1),
                                     gammaGreen) * dValue,
                                 1.0) * (double)UINT16_MAX);

        if (gammaBlue == 1.0 && dValue == 1.0)
            vecBlue.append((double)i / (double)(rampsize - 1) * (double)UINT16_MAX);
        else
            vecBlue.append(dmin(pow((double)i/(double)(rampsize - 1),
                                    gammaBlue) * dValue,
                                1.0) * (double)UINT16_MAX);
    }

    m_outputConfiguration->setColorCurves(pOutputDevice, vecRed, vecGreen, vecBlue);
    m_outputConfiguration->apply();

    outputBrightnessMap[findNameForOutputDevice(pOutputDevice)] = dValue;
}

void WaylandOutput::SetColorTemperature(int nValue)
{
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
        OutputDevice::ColorCurves colorCurves = pOutputDevice->colorCurves();

        m_outputConfiguration = m_outputManagement->createConfiguration(this);
        int rampsize = colorCurves.red.size();

        QVector<quint16> vecRed;
        QVector<quint16> vecGreen;
        QVector<quint16> vecBlue;

        // linear default state
        for (int i = 0; i < rampsize; i++) {
            uint16_t value = (double)i / rampsize * (UINT16_MAX + 1);
            vecRed.push_back(value);
            vecGreen.push_back(value);
            vecBlue.push_back(value);
        }

        // approximate white point
        float whitePoint[3];
        float alpha = (nValue % 100) / 100.;
        int bbCIndex = ((nValue - 1000) / 100) * 3;
        whitePoint[0] = (1. - alpha) * blackbodyColor[bbCIndex] + alpha * blackbodyColor[bbCIndex + 3];
        whitePoint[1] = (1. - alpha) * blackbodyColor[bbCIndex + 1] + alpha * blackbodyColor[bbCIndex + 4];
        whitePoint[2] = (1. - alpha) * blackbodyColor[bbCIndex + 2] + alpha * blackbodyColor[bbCIndex + 5];

        for (int i = 0; i < rampsize; i++) {
            vecRed[i] = (double)vecRed[i] / (UINT16_MAX+1) * whitePoint[0] * (UINT16_MAX+1);
            vecGreen[i] = (double)vecGreen[i] / (UINT16_MAX+1) * whitePoint[1] * (UINT16_MAX+1);
            vecBlue[i] = (double)vecBlue[i] / (UINT16_MAX+1) * whitePoint[2] * (UINT16_MAX+1);
        }
        m_outputConfiguration->setColorCurves(pOutputDevice, vecRed, vecGreen, vecBlue);
        m_outputConfiguration->apply();
    }

    if (m_strCCTMode == manualmode) {
        m_nCCTValue = nValue;
    }
}

// set automatic color temperature adjustment
void WaylandOutput::SetMethodAdjustCCT(int nAdjustMethod)
{
    m_strCCTMode = nAdjustMethod;

    if (m_strCCTMode != manualmode) {
        SetColorTemperature(6500);
    }
}

void WaylandOutput::SetPrimary(QString strOutputName)
{
    m_strPrimaryScreenName = strOutputName;
}

void WaylandOutput::SwitchMode(uchar uMode, QString strName)
{
    if (!findOutputDeviceForName(strName) || uMode > displayMode::screenShowAloneMode) {
        return;
    }

    m_outputConfiguration = m_outputManagement->createConfiguration(this);

    switch (uMode) {
    case screenExtendMode: {
        int x = 0;
        for (int i = 0; i < m_outputDeviceLst.count(); i++) {
            OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
            m_outputConfiguration->setEnabled(pOutputDevice, OutputDevice::Enablement::Enabled);
            m_outputConfiguration->setMode(pOutputDevice, pOutputDevice->modes().first().flags);
            m_outputConfiguration->setTransform(pOutputDevice, pOutputDevice->transform());
            if (i == 0) {
                m_outputConfiguration->setPosition(pOutputDevice, QPoint(0, 0));
            } else {
                x += pOutputDevice->geometry().width();
                m_outputConfiguration->setPosition(pOutputDevice, QPoint(x, 0));
            }
        }
        m_outputConfiguration->apply();
        nDisplayMode = screenExtendMode;
    }
        break;
    case screenCopyMode: {
        for (int i = 0; i < m_outputDeviceLst.count(); i++) {
            OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
            m_outputConfiguration->setEnabled(pOutputDevice, OutputDevice::Enablement::Enabled);
            m_outputConfiguration->setMode(pOutputDevice, pOutputDevice->modes().first().flags);
            m_outputConfiguration->setTransform(pOutputDevice, pOutputDevice->transform());
            m_outputConfiguration->setPosition(pOutputDevice,QPoint(0, 0));
        }
        m_outputConfiguration->apply();
        nDisplayMode = screenCopyMode;
    }
        break;
    case screenShowAloneMode: {
         for (int i = 0; i < m_outputDeviceLst.count(); i++) {
            OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
            if (findNameForOutputDevice(pOutputDevice) == strName) {
                m_outputConfiguration->setEnabled(pOutputDevice, OutputDevice::Enablement::Enabled);
                m_outputConfiguration->setMode(pOutputDevice, pOutputDevice->modes().first().flags);
                m_outputConfiguration->setTransform(pOutputDevice, pOutputDevice->transform());
                m_outputConfiguration->setPosition(pOutputDevice,QPoint(0, 0));
            } else {
                m_outputConfiguration->setEnabled(pOutputDevice, OutputDevice::Enablement::Disabled);
                m_outputConfiguration->setMode(pOutputDevice, pOutputDevice->modes().first().flags);
            }
         }
         m_outputConfiguration->apply();
         nDisplayMode = screenShowAloneMode;
    }
        break;
    default:
        return;
    }
}

OutputDevice* WaylandOutput::findOutputDeviceForName(QString strName)
{
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDevice *pOutputDevice = m_outputDeviceLst.at(i);
        if (pOutputDevice->model().startsWith(strName)) {
            return pOutputDevice;
        }
    }
    return nullptr;
}

QString WaylandOutput::findNameForOutputDevice(OutputDevice *pOutputDevice)
{
    QStringList strResultLst;
    QString strOutputName = pOutputDevice->model();
    QStringList outputname = strOutputName.split("-");
    for (int i = 0; i < outputname.count(); i++) {
        QString strName = outputname.at(i);
        if (strName.front().isNumber()) {
          strResultLst << strName;
          break;
        }
        strResultLst << strName;
    }
    return strResultLst.join("-");
}

Output* WaylandOutput::findOutputForName(QString strName)
{
    for (int i = 0; i < m_OutputList.count(); i++) {
        Output *pOutput = m_OutputList.at(i);
        if (pOutput->model().startsWith(strName)) {
            return pOutput;
        }
    }
    return nullptr;
}

QString WaylandOutput::findNameForOutput(Output *pOutput)
{
    QStringList strResultLst;
    QString strOutputName = pOutput->model();
    QStringList outputname = strOutputName.split("-");
    for (int i = 0; i < outputname.count(); i++) {
        QString strName = outputname.at(i);
        if (strName.front().isNumber()) {
          strResultLst << strName;
          break;
        }
        strResultLst << strName;
    }
    return strResultLst.join("-");
}

void WaylandOutput::setDpmsStatus(Dpms::Mode mode, Output *pOutput)
{
    if (!pOutput) {
        for (int i = 0; i < m_OutputList.count(); i++) {
            Output *pOutput = m_OutputList.at(i);
            Dpms *pDpms = m_dpmsManager->getDpms(pOutput, this);
            pDpms->requestMode(mode);
        }
    } else {
        Dpms *pDpms = m_dpmsManager->getDpms(pOutput, this);
        pDpms->requestMode(mode);
    }
}
