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

#include "wayland_screen.h"

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
#include <QRect>

#include "wayland/wayland_screen.h"
#include "DWayland/Client/outputdevice_v2.h"
#include "screenadaptor.h"

#define DBUS_DEEPIN_WM_SERVICE "org.kde.KWin"
#define DBUS_DEEPIN_WM_OBJ "/KWin"
#define DBUS_DEEPIN_WM_INTF "org.kde.KWin"

#define screenPath "/com/deepin/kwin/Display/Screen"
#define screenInterface "com.deepin.kwin.Display.Screen"

bool isX11Platform()
{
    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));

    if (XDG_SESSION_TYPE != QLatin1String("x11")) {
        return false;
    }
    return true;
}

WaylandScreen::WaylandScreen(OutputDeviceV2 *pOutputDevice, OutputManagementV2 *pOutputManagementV2, QObject *parent)
    : AbstractScreen(parent)
{
    new ScreenAdaptor(this);

    m_outputManagement = pOutputManagementV2;
    m_pOutputDevice = pOutputDevice;
    QString strScreenPath = QString("%1%2").arg("/com/deepin/kwin/Display/Screen_").arg(QString(pOutputDevice->uuid().left(8)));
    QDBusConnection::sessionBus().registerObject(strScreenPath, this, QDBusConnection::ExportAllContents);
    m_modesList = Modes();
}

WaylandScreen::~WaylandScreen()
{

}

int WaylandScreen::ColorTemperatureManual()
{
    return int();
}

double WaylandScreen::Brightness()
{
    return double();
}

QMap<QString, QVariant> WaylandScreen::BestDisplayMode()
{
    for (int i = 0; i < m_pOutputDevice->modes().count(); i++) {
        DeviceModeV2 *pDeviceModeV2 = m_pOutputDevice->modes().at(i);
        QMap<QString, QVariant> modesInfo;
        modesInfo["id"] = i+1;
        modesInfo["width"] = pDeviceModeV2->size().width();
        modesInfo["height"] = pDeviceModeV2->size().height();
        modesInfo["refreshRate"] = pDeviceModeV2->refreshRate() / 1000.00;
        if (pDeviceModeV2->preferred()) {
            return modesInfo;
        }
    }
    return QMap<QString, QVariant>();
}

int WaylandScreen::ScreenWidth()
{
    return m_pOutputDevice->geometry().width();
}

int WaylandScreen::ScreenHeight()
{
    return m_pOutputDevice->geometry().height();
}

double WaylandScreen::RefreshRate()
{
    return m_pOutputDevice->currentMode()->refreshRate() / 1000.00;
}

bool WaylandScreen::Connected()
{
    return bool(m_pOutputDevice->enabled());
}

QString WaylandScreen::Model()
{
    return m_pOutputDevice->model();
}

QVariantList WaylandScreen::Rotations()
{
    QVariantList rotationsList;
    rotationsList << 0 << 1 <<2 << 3;
    return rotationsList;
}

int WaylandScreen::X()
{
    return m_pOutputDevice->globalPosition().x();
}

int WaylandScreen::Y()
{
    return m_pOutputDevice->globalPosition().y();
}

QString WaylandScreen::ID()
{
    return QString(m_pOutputDevice->uuid().left(8));
}

bool WaylandScreen::Enabled()
{
    return bool(m_pOutputDevice->enabled());
}

int WaylandScreen::PhysicalWidth()
{
    return  m_pOutputDevice->physicalSize().width();
}

int WaylandScreen::PhysicalHeight()
{
    return m_pOutputDevice->physicalSize().height();
}

int WaylandScreen::Rotation()
{
    return int(m_pOutputDevice->transform());
}

QString WaylandScreen::Name()
{
    QStringList strResultLst;
    QString strOutputName = m_pOutputDevice->model();
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

QString WaylandScreen::Manufacturer()
{
    return m_pOutputDevice->manufacturer();
}

QVariantList WaylandScreen::Modes()
{
    QVariantList modesList;
    for (int i = 0; i < m_pOutputDevice->modes().count(); i++) {
        DeviceModeV2 *pDeviceModeV2 = m_pOutputDevice->modes().at(i);
        QMap<QString, QVariant> modesInfo;
        modesInfo["id"] = i+1;
        modesInfo["width"] = pDeviceModeV2->size().width();
        modesInfo["height"] = pDeviceModeV2->size().height();
        modesInfo["refreshRate"] = pDeviceModeV2->refreshRate() / 1000.00;
        modesList << modesInfo;
    }
    m_modesList = modesList;
    return m_modesList;
}

QVariantList WaylandScreen::CurrentMode()
{
    QVariantList modesList;
    for (int i = 0; i < m_modesList.count(); i++) {
        QMap<QString, QVariant> modesInfo = m_modesList.at(i).toMap();
        if (modesInfo["width"].toInt() == m_pOutputDevice->currentMode()->size().width() &&
                modesInfo["height"].toInt() == m_pOutputDevice->currentMode()->size().height() &&
                modesInfo["refreshRate"].toDouble() == m_pOutputDevice->currentMode()->refreshRate() / 1000.00){
            modesList << modesInfo;
        }
    }
    return modesList;
}

void WaylandScreen::SetResolution(int nWidth, int nHeight)
{
    int nModeId = 0;
    for (int i = 0; i < m_modesList.count(); i++) {
        QMap<QString, QVariant> modesInfo = m_modesList.at(i).toMap();
        if (modesInfo["width"].toInt() == nWidth && modesInfo["height"].toInt() == nHeight){
            nModeId = modesInfo["id"].toInt() - 1;
        }
    }

    if (nModeId < 0) {
        sendErrorReply(QDBusError::ErrorType::Failed, "No support this Resolution!");
        return;
    }

    m_outputConfiguration = m_outputManagement->createConfiguration(this);
    m_outputConfiguration->setMode(m_pOutputDevice, nModeId);
    m_outputConfiguration->apply();
}

void WaylandScreen::SetRefreshRate(double dRefreshRate)
{
    int nWidth = m_pOutputDevice->currentMode()->size().width();
    int nHeight = m_pOutputDevice->currentMode()->size().height();

    for (int i = 0; i < m_modesList.count(); i++) {
        QMap<QString, QVariant> modesInfo = m_modesList.at(i).toMap();
        if (modesInfo["width"].toInt() == nWidth && modesInfo["height"].toInt() == nHeight && modesInfo["refreshRate"].toInt() / 1000.00 == dRefreshRate){
            m_outputConfiguration->setMode(m_pOutputDevice, modesInfo["id"].toInt());
            m_outputConfiguration->apply();
            return;
        }
    }
}

void WaylandScreen::SetRotation(int nRotation)
{
    if (nRotation > 3 || nRotation < 0) {
        sendErrorReply(QDBusError::ErrorType::Failed, "No support this Rotation!");
        return;
    }
    m_outputConfiguration->setTransform(m_pOutputDevice, KWayland::Client::OutputDeviceV2::Transform(nRotation));
    m_outputConfiguration->apply();
}

void WaylandScreen::SetScreenPosition(int x, int y)
{
    m_outputConfiguration->setPosition(m_pOutputDevice, QPoint(x, y));
    m_outputConfiguration->apply();
}

void WaylandScreen::SetBrightness(double dBrightness)
{
////    OutputDevice::ColorCurves colorCurves = pOutputDevice->colorCurves();

    m_outputConfiguration = m_outputManagement->createConfiguration(this);
//    int rampsize = colorCurves.red.size();

    int rampsize = 1;
    float gammaRed = 1.0;
    float gammaGreen = 1.0;
    float gammaBlue = 1.0;
    QVector<quint16> vecRed;
    QVector<quint16> vecGreen;
    QVector<quint16> vecBlue;

    for (int i = 0; i < rampsize; i++) {
        if (gammaRed == 1.0 && dBrightness == 1.0)
            vecRed.append((double)i / (double)(rampsize - 1) * (double)UINT16_MAX);
        else
            vecRed.append(dmin(pow((double)i/(double)(rampsize - 1),
                                   gammaRed) * dBrightness,
                               1.0) * (double)UINT16_MAX);

        if (gammaGreen == 1.0 && dBrightness == 1.0)
            vecGreen.append((double)i / (double)(rampsize - 1) * (double)UINT16_MAX);
        else
            vecGreen.append(dmin(pow((double)i/(double)(rampsize - 1),
                                     gammaGreen) * dBrightness,
                                 1.0) * (double)UINT16_MAX);

        if (gammaBlue == 1.0 && dBrightness == 1.0)
            vecBlue.append((double)i / (double)(rampsize - 1) * (double)UINT16_MAX);
        else
            vecBlue.append(dmin(pow((double)i/(double)(rampsize - 1),
                                    gammaBlue) * dBrightness,
                                1.0) * (double)UINT16_MAX);
    }

    m_outputConfiguration->setColorCurves(m_pOutputDevice, vecRed, vecGreen, vecBlue);
    m_outputConfiguration->apply();
}

void WaylandScreen::SetColorTemperature(int nColorTemperature)
{

}

QString WaylandScreen::CurrentScalingMode()
{
    return QString();
}

void WaylandScreen::setScalingMode(int nMode)
{

}

QVariantList WaylandScreen::SupportScalingMode()
{
    return QVariantList();
}
