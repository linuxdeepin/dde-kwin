/*
 * Copyright (C) 2022 Uniontech Technology Co., Ltd.
 *
 * Author:     xinbo wang <wangxinbo@uniontech.com>
 *
 * Maintainer: xinbo wang <wangxinbo@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wayland_output.h"

#include <QObject>
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
#include <QPoint>

#include "abstract_output.h"
#include "wayland/wayland_screen.h"
#include "DWayland/Client/output.h"
#include "DWayland/Client/outputdevice_v2.h"
#include "DWayland/Client/outputdevice.h"
#include "DWayland/Client/pointer.h"
#include "DWayland/Client/compositor.h"

#define DBUS_DEEPIN_WM_SERVICE "org.kde.KWin"
#define DBUS_DEEPIN_WM_OBJ "/KWin"
#define DBUS_DEEPIN_WM_INTF "org.kde.KWin"

#define DBUS_DEEPIN_DOCK_SERVICE "com.deepin.dde.daemon.Dock"
#define DBUS_DEEPIN_DOCK_OBJ "/com/deepin/dde/daemon/Dock"
#define DBUS_DEEPIN_DOCK_INTF "com.deepin.dde.daemon.Dock"

#define BLACK_LIGHT_PATH "/sys/class/backlight"

//using namespace KWayland::Client;

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
    connect(registry, &Registry::outputManagementV2Announced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_outputManagement = registry->createOutputManagementV2(name, version, this);
            m_outputManagement->setEventQueue(m_eventQueue);
        }
    );

    connect(registry, &Registry::outputDeviceV2Announced, this,
        [this, registry] (quint32 name, quint32 version) {
            OutputDeviceV2 *pOutputDevice = registry->createOutputDeviceV2(name, version, this);
            if (pOutputDevice) {
                m_outputDeviceLst << pOutputDevice;
                emit outputAdded();

                connect(pOutputDevice, &OutputDeviceV2::changed, this, [=](){
                });

                connect(pOutputDevice, &OutputDeviceV2::removed, this, [=](){

                    QString strScreenPath = QString("%1%2").arg("/com/deepin/kwin/Display/Screen_").arg(QString(pOutputDevice->uuid().left(8)));
                    QDBusConnection::sessionBus().unregisterObject(strScreenPath, QDBusConnection::UnregisterTree);

                    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
                        if (m_outputDeviceLst.at(i) == pOutputDevice) {
                            m_outputDeviceLst.removeAt(i);
                            break;
                        }
                    }

                    QMap<QString, QVariant> OutputDeviceInfo;
                    OutputDeviceInfo["screenName"] = findNameForOutputDevice(pOutputDevice);
                    OutputDeviceInfo["uuid"] = QString(pOutputDevice->uuid().left(8));
                    emit outputRemoved(OutputDeviceInfo);

                    WaylandScreen *pWaylandScreen = pOutputDevice->findChild<WaylandScreen *>(QString(pOutputDevice->uuid().left(8)));
                    if (pWaylandScreen) {
                        delete pWaylandScreen;
                        pWaylandScreen = nullptr;
                    }

                });

                connect(pOutputDevice, &OutputDeviceV2::done, this, [=](){
                    WaylandScreen *pWaylandScreen = pOutputDevice->findChild<WaylandScreen *>(QString(pOutputDevice->uuid().left(8)));
                    if (!pWaylandScreen) {
                        pWaylandScreen = new WaylandScreen(pOutputDevice, m_outputManagement, pOutputDevice);
                        pWaylandScreen->setObjectName(QString(pOutputDevice->uuid().left(8)));
                    }
                });
            }
        }
    );

    connect(registry, &Registry::compositorAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_compositor = registry->createCompositor(name, version, this);
            m_compositor->setEventQueue(m_eventQueue);
            m_Surface = m_compositor->createSurface(m_compositor);
    }
    );

    connect(registry, &Registry::seatAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_pSeat = registry->createSeat(name, version, this);
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

QVariantList WaylandOutput::Touchscreens()
{
    QVariantList touchScreenList;
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
                for (int i = 0; i < map.keys().count(); i++) {
                    if (map.keys().at(i) == "TouchDevice") {
                        OutputDeviceV2 *pOutputDeviceV2 = findOutputDeviceForUuid(map["ScreenUuid"].toString());
                        if (pOutputDeviceV2) {
                            map["ScreenName"] = findNameForOutputDevice(pOutputDeviceV2);
                            touchScreenList << map;
                        }
                    }
                }
            }
        }
    }
    return touchScreenList;
}

QString WaylandOutput::PrimaryScreenName()
{
    return m_strPrimaryScreenName;
}

QStringList WaylandOutput::Monitors()
{
    QStringList strMonitorsList;

    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDeviceV2 *pOutputDevice = m_outputDeviceLst.at(i);
        strMonitorsList << QString("%1%2").arg("/com/deepin/kwin/Display/Screen_").arg(QString(pOutputDevice->uuid().left(8)));
    }

    return strMonitorsList;
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

ushort WaylandOutput::DisplayWidth()
{
    int displayWidth = 0;
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDeviceV2 *pOutputDevice = m_outputDeviceLst.at(i);
        if (pOutputDevice->geometry().x() + pOutputDevice->geometry().width() > displayWidth) {
            displayWidth = pOutputDevice->geometry().x() + pOutputDevice->geometry().width();
        }
    }
    return displayWidth;
}

ushort WaylandOutput::DisplayHeight()
{
    int displayheight = 0;
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDeviceV2 *pOutputDevice = m_outputDeviceLst.at(i);
        if (pOutputDevice->geometry().y() + pOutputDevice->geometry().height() > displayheight) {
            displayheight = pOutputDevice->geometry().y() + pOutputDevice->geometry().height();
        }
    }
    return displayheight;
}

QMap<QString, QVariant> WaylandOutput::TouchMap()
{
    QMap<QString, QVariant> touchMap;
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
                for (int i = 0; i < map.keys().count(); i++) {
                    if (map.keys().at(i) == "TouchDevice") {
                        OutputDeviceV2 *pOutputDeviceV2 = findOutputDeviceForUuid(map["ScreenUuid"].toString());
                        if (pOutputDeviceV2) {
                            map["ScreenName"] = findNameForOutputDevice(pOutputDeviceV2);
                            touchMap[QString(pOutputDeviceV2->uuid().left(8))] = findNameForOutputDevice(pOutputDeviceV2);
                        }
                    }
                }
            }
        }
    }
    return touchMap;
}

uchar WaylandOutput::DisplayMode()
{
    if (nDisplayMode == 0) {
        if (m_outputDeviceLst.count() > 1) {
            for (int i = 0; i < m_outputDeviceLst.count(); i++) {
                OutputDeviceV2 *pOutputDevice = m_outputDeviceLst.at(i);
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

void WaylandOutput::SetPrimaryScreen(QString strOutputName)
{
    OutputDeviceV2 *pOutputDevice = findOutputDeviceForName(m_strPrimaryScreenName);
    m_outputConfiguration = m_outputManagement->createConfiguration(this);
    if (pOutputDevice) {
        m_outputConfiguration->setPrimaryOutput(pOutputDevice);
        m_strPrimaryScreenName = strOutputName;
    } else {
        sendErrorReply(QDBusError::ErrorType::Failed, "No such as screen name exists!");
        return;
    }
}

void WaylandOutput::showCursor()
{
//    if (m_Surface) {
//        Pointer *p = m_pSeat->createPointer(m_pSeat);
//        if (p) {
//            p->setCursor(m_Surface, QPoint(0 ,0));
//        }
//    }
}

void WaylandOutput::hideCursor()
{
//    Pointer *p = m_pSeat->createPointer(m_pSeat);
//    if (p) {
//        p->hideCursor();
//    }
}

void WaylandOutput::SetDisplayMode(uchar uMode, QVariantList screenInfoList)
{
    m_outputConfiguration = m_outputManagement->createConfiguration(this);

    for (int i = 0; i < screenInfoList.count(); i++) {
        QMap<QString, QVariant> screenInfo = screenInfoList.at(i).toMap();
        QString strName = screenInfo["name"].toString();
        QString strUUid = screenInfo["uuid"].toString();
        bool bEnable = screenInfo["enable"].toBool();
        int nRotation = screenInfo["rotation"].toInt();
        int x = screenInfo["x"].toInt();
        int y = screenInfo["y"].toInt();
        int nMode = screenInfo["modeId"].toInt();
        bool bPrimary = screenInfo["primary"].toBool();
        double dBrightness = screenInfo["brightness"].toDouble();

        OutputDeviceV2 *pOutputDeviceV2 = findOutputDeviceForName(strName);
        if (!pOutputDeviceV2) {
            sendErrorReply(QDBusError::ErrorType::Failed, QString("No named %1 screen exists!").arg(strName));
            return;
        }

        WaylandScreen *pWaylandScreen = pOutputDeviceV2->findChild<WaylandScreen *>(QString(pOutputDeviceV2->uuid().left(8)));
        if (!pWaylandScreen) {
            sendErrorReply(QDBusError::ErrorType::Failed, QString("%1 uuid not exists!").arg(strName));
            return;
        }
        pWaylandScreen->SetBrightness(dBrightness);

        m_outputConfiguration->setMode(pOutputDeviceV2, nMode - 1);
        m_outputConfiguration->setEnabled(pOutputDeviceV2, OutputDeviceV2::Enablement(bEnable));
        m_outputConfiguration->setTransform(pOutputDeviceV2, OutputDeviceV2::Transform(nRotation));
        m_outputConfiguration->setPosition(pOutputDeviceV2, QPoint(x, y));

        if (bPrimary) {
            m_outputConfiguration->setPrimaryOutput(pOutputDeviceV2);
            m_strPrimaryScreenName = strName;
        }
        m_outputConfiguration->apply();
    }
    nDisplayMode = uMode;
}

void WaylandOutput::SetColorTemperatureMode(int nValue)
{
    m_nCCTMode = nValue;
}

int WaylandOutput::ColorTemperatureMode()
{
    return m_nCCTMode;
}

double WaylandOutput::Scale()
{
    return m_dScale;
}

void WaylandOutput::SetScale(double dScale)
{
    if (dScale > 2.5) {
        m_dScale = 2.5;
    } if (dScale < 0.5) {
        m_dScale = 0.5;
    } else {
        m_dScale = dScale;
    }

    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDeviceV2 *pOutputDevice = m_outputDeviceLst.at(i);
        m_outputConfiguration->setScaleF(pOutputDevice, m_dScale);
    }
    m_outputConfiguration->apply();
}

void WaylandOutput::mapToOutput(int nDeviceId, QString strOutputName)
{

}

OutputDeviceV2* WaylandOutput::findOutputDeviceForName(QString strName)
{
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDeviceV2 *pOutputDevice = m_outputDeviceLst.at(i);
        if (pOutputDevice->model().startsWith(strName)) {
            return pOutputDevice;
        }
    }
    return nullptr;
}

QString WaylandOutput::findNameForOutputDevice(OutputDeviceV2 *pOutputDevice)
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

OutputDeviceV2* WaylandOutput::findOutputDeviceForUuid(QString strUuid)
{
    for (int i = 0; i < m_outputDeviceLst.count(); i++) {
        OutputDeviceV2 *pOutputDevice = m_outputDeviceLst.at(i);
        if (pOutputDevice->model().contains(strUuid)) {
            return pOutputDevice;
        }
    }
    return nullptr;
}
