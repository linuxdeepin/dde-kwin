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

#include "x11_screen.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>
#include <QDebug>
#include <QDBusReply>
#include <QCryptographicHash>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <cstdio>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <math.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>
#include <X11/Xatom.h>

#include "../../output/x11/xcbwrapper.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

Display *m_pDisplay = nullptr;

X11Screen::X11Screen(xcb_connection_t* pXConnection, xcb_screen_t* pXFirstScreen, xcb_window_t XWindowDummy, QString &strName, xcb_randr_crtc_t xcb_randr_crtc_t, QObject *parent)
    : AbstractScreen(parent)
{
    m_pDisplay = XOpenDisplay(nullptr);

    m_pXConnection = pXConnection;
    m_pXFirstScreen = pXFirstScreen;
    m_XWindowDummy = XWindowDummy;
    m_xcb_randr_crtc_t = xcb_randr_crtc_t;
    m_strUuid = strName;

    QString strScreenPath = QString("%1%2").arg("/com/deepin/kwin/Display/Screen_").arg(strName);
    QDBusConnection::sessionBus().registerObject(strScreenPath, this, QDBusConnection::ExportAllContents);

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    xcb_randr_get_screen_resources_reply_t *reply =
            xcb_randr_get_screen_resources_reply(m_pXConnection,
                                                 xcb_randr_get_screen_resources(m_pXConnection, screen->root),
                                                 NULL);

    xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_modes(reply);

    xcb_randr_get_screen_resources_current_reply_t *currentReply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(currentReply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(currentReply);

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);

        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        QByteArray outputEdidInfo = outputEdid(randr_outputs[i]);

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        xcb_randr_mode_t *outputModes = xcb_randr_get_output_info_modes(outputInfoReply);

        char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);
        QString strScreenName = formatOutputName(QString::fromUtf8(monitorName));

        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(strScreenName.toStdString().c_str());
        QString strUuid = QString::fromStdString(hash.result().toHex().left(8).toStdString());

        if (strUuid == m_strUuid) {
            m_strScreenName = strScreenName;
            m_strManufacturer = extractEisaId(outputEdidInfo);
            m_strSerialNumber = extractSerialNumber(outputEdidInfo);
            m_strMonitorName = extractMonitorName(outputEdidInfo);

            for (int i = 0; i < outputInfoReply->num_modes; i++) {
                 for (int j = 0; j < reply->num_modes; j++) {
                     if (modes[j].id != outputModes[i]) {
                         continue;
                     }

                     QMap<QString, QVariant> modesInfo;
                     modesInfo["id"] = modes[j].id;
                     modesInfo["width"] = modes[j].width;
                     modesInfo["height"] = modes[j].height;
                     modesInfo["refreshRate"] =((double)modes[j].dot_clock / (double)(modes[j].htotal * modes[j].vtotal));
                     m_modesList << modesInfo;
                 }
            }
        }
        free(outputInfoReply);
        free(crtc);
    }
    free(reply);
    free(currentReply);

    getScalingModeInfo();

}

X11Screen:: ~X11Screen()
{
    XCloseDisplay(m_pDisplay);
}

int X11Screen::ColorTemperatureManual()
{
    return m_nColor;
}

double X11Screen::Brightness()
{
    return m_dBrightness;
}

QMap<QString, QVariant> X11Screen::BestDisplayMode()
{
    return m_modesList.first().toMap();
}

int X11Screen::ScreenWidth()
{
    int nScreenWidth = 0;
    for (int i =0; i< CurrentMode().count(); i++) {
        QMap<QString, QVariant> modeInfo = CurrentMode().at(i).toMap();
        nScreenWidth = modeInfo["width"].toInt();
        break;
    }
    return nScreenWidth;
}

int X11Screen::ScreenHeight()
{
    int nScreenHeighht = 0;
    for (int i =0; i< CurrentMode().count(); i++) {
        QMap<QString, QVariant> modeInfo = CurrentMode().at(i).toMap();
        nScreenHeighht = modeInfo["height"].toInt();
        break;
    }
    return nScreenHeighht;
}

double X11Screen::RefreshRate()
{
    double refreshRate = 60.00;
    for (int i =0; i< CurrentMode().count(); i++) {
        QMap<QString, QVariant> modeInfo = CurrentMode().at(i).toMap();
        refreshRate = modeInfo["refreshRate"].toDouble();
        break;
    }
    return refreshRate;
}

bool X11Screen::Connected()
{
    bool connected = true;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    xcb_randr_get_screen_resources_reply_t *reply =
            xcb_randr_get_screen_resources_reply(m_pXConnection,
                                                 xcb_randr_get_screen_resources(m_pXConnection, screen->root),
                                                 NULL);

    xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_modes(reply);

    xcb_randr_get_screen_resources_current_reply_t *screenReply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(screenReply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(screenReply);

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE)
            continue;

        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(m_strScreenName.toStdString().c_str());
        QString strUuid = QString::fromStdString(hash.result().toHex().left(8).toStdString());

        if (strUuid == m_strUuid) {
            if (outputInfoReply->connection == XCB_RANDR_CONNECTION_CONNECTED) {
                connected = true;
            } else {
                connected = false;
            }
            break;
        }
        free(outputInfoReply);
    }
    free(screenReply);
    free(reply);

    return connected;
}

QString X11Screen::Model()
{
    return m_strScreenName + "-" + m_strMonitorName;
}

QVariantList X11Screen::Rotations()
{
    qDebug() << " ----- Rotations";

    QVariantList list;
    list << 0 << 1 << 2 << 3;
    return list;
}

int X11Screen::X()
{

    qDebug() << " ----- X ";


    int x = 0;
    xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                          xcb_randr_get_crtc_info(m_pXConnection, m_xcb_randr_crtc_t, 0), NULL);
    x = crtc->x;
    free(crtc);

    return x;
}

int X11Screen::Y()
{
    int y = 0;
    xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                          xcb_randr_get_crtc_info(m_pXConnection, m_xcb_randr_crtc_t, 0), NULL);
    y = crtc->y;
    free(crtc);

    return y;
}

QString X11Screen::ID()
{
    return m_strSerialNumber;
}

bool X11Screen::Enabled()
{
    return Connected();
}

int X11Screen::PhysicalWidth()
{
    int nPhysicalWidth = 0;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    xcb_randr_get_screen_resources_reply_t *reply =
            xcb_randr_get_screen_resources_reply(m_pXConnection,
                                                 xcb_randr_get_screen_resources(m_pXConnection, screen->root),
                                                 NULL);

    xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_modes(reply);

    xcb_randr_get_screen_resources_current_reply_t *screenReply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(screenReply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(screenReply);

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE)
            continue;

        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(m_strScreenName.toStdString().c_str());
        QString strUuid = QString::fromStdString(hash.result().toHex().left(8).toStdString());

        if (strUuid == m_strUuid) {
            nPhysicalWidth = outputInfoReply->mm_width;
            break;
        }
        free(outputInfoReply);
    }
    free(screenReply);
    free(reply);

    return nPhysicalWidth;
}

int X11Screen::PhysicalHeight()
{
    int nPhysicalHeight = 0;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    xcb_randr_get_screen_resources_reply_t *reply =
            xcb_randr_get_screen_resources_reply(m_pXConnection,
                                                 xcb_randr_get_screen_resources(m_pXConnection, screen->root),
                                                 NULL);

    xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_modes(reply);

    xcb_randr_get_screen_resources_current_reply_t *screenReply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(screenReply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(screenReply);

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE)
            continue;

        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(m_strScreenName.toStdString().c_str());
        QString strUuid = QString::fromStdString(hash.result().toHex().left(8).toStdString());

        if (strUuid == m_strUuid) {
            nPhysicalHeight = outputInfoReply->mm_height;
            break;
        }
        free(outputInfoReply);
    }
    free(screenReply);
    free(reply);

    return nPhysicalHeight;
}

int X11Screen::Rotation()
{
    int nRotation = 0;

    xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                          xcb_randr_get_crtc_info(m_pXConnection, m_xcb_randr_crtc_t, 0), NULL);
    nRotation = crtc->rotation;
    free(crtc);

    return nRotation;
}

QString X11Screen::Name()
{
    return m_strScreenName;
}

QString X11Screen::Manufacturer()
{
    // fix me: manufacturer ues three bytes memory in EDID, so it can cut off the real name.
    // for example, Dell in EDID is Del.
    return m_strManufacturer;
}

QVariantList X11Screen::Modes()
{
    return m_modesList;
}

QVariantList X11Screen::CurrentMode()
{
    QVariantList currentModeList;

    xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                          xcb_randr_get_crtc_info(m_pXConnection, m_xcb_randr_crtc_t, 0), NULL);
    for (int i = 0; i < m_modesList.count(); i++) {
         QMap<QString, QVariant> modeInfo = m_modesList.at(i).toMap();
         if (modeInfo["id"].toInt() == crtc->mode) {
            currentModeList << modeInfo;
         }
    }
    free(crtc);
    return currentModeList;
}

QString X11Screen::CurrentScalingMode()
{
    getScalingModeInfo();
    return currentScalingMode;
}

QVariantList X11Screen::SupportScalingMode()
{
    return m_supportScalingModeList;
}

void X11Screen::SetBrightness(double dBrightness)
{
    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_crtc_gamma_cookie_t crtcGammaCookie = {};
    crtcGammaCookie = xcb_randr_get_crtc_gamma(m_pXConnection, m_xcb_randr_crtc_t);

    xcb_randr_get_crtc_gamma_reply_t *crtcGammaReply = {};
    crtcGammaReply = xcb_randr_get_crtc_gamma_reply(m_pXConnection, crtcGammaCookie, &xcbError);

    uint16_t *gammaRed = xcb_randr_get_crtc_gamma_red(crtcGammaReply);
    uint16_t *gammaGreen = xcb_randr_get_crtc_gamma_green(crtcGammaReply);
    uint16_t *gammaBlue = xcb_randr_get_crtc_gamma_blue(crtcGammaReply);

    xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie = xcb_randr_get_crtc_gamma_size(m_pXConnection, m_xcb_randr_crtc_t);
    xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply = xcb_randr_get_crtc_gamma_size_reply(m_pXConnection, gamma_size_cookie, &xcbError);

    unsigned int ramp_size = gamma_size_reply->size;

    colorrampFill(gammaRed, gammaGreen, gammaBlue, ramp_size, m_nColor, dBrightness);

    xcb_void_cookie_t gamma_set_cookie = xcb_randr_set_crtc_gamma(m_pXConnection, m_xcb_randr_crtc_t,
                                                                  ramp_size, gammaRed,
                                                                  gammaGreen, gammaBlue);

    free(crtcGammaReply);
    free(gamma_size_reply);
    xcb_flush(m_pXConnection);
}

void X11Screen::SetResolution(int nWidth, int nHeight)
{
    int nModeId = -1;
    for (int i = 0; i < m_modesList.count(); i++) {
        QMap<QString, QVariant> modesInfo = m_modesList.at(i).toMap();
        int Id = modesInfo["id"].toInt();
        int width = modesInfo["width"].toInt();
        int height = modesInfo["width"].toInt();
        double drefreshRate = modesInfo["refreshRate"].toDouble();

        if (nWidth == width && nHeight == height && drefreshRate == RefreshRate()) {
            nModeId = Id;
            break;
        }
    }

    if (nModeId < 0)
        return;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                          xcb_randr_get_crtc_info(m_pXConnection, m_xcb_randr_crtc_t, 0), NULL);

    xcb_randr_output_t *poutput = xcb_randr_get_crtc_info_outputs(crtc);
    xcb_randr_set_crtc_config_cookie_t crtcConfigCookie = {};
    crtcConfigCookie = xcb_randr_set_crtc_config(m_pXConnection, m_xcb_randr_crtc_t, config_timestamp, config_timestamp, X(), Y(), nModeId, Rotation(), 1, poutput);

    free(crtc);
    xcb_flush(m_pXConnection);
}

void X11Screen::SetRefreshRate(double dRefreshRate)
{
    int nModeId = -1;
    for (int i = 0; i < m_modesList.count(); i++) {
        QMap<QString, QVariant> modesInfo = m_modesList.at(i).toMap();
        int Id = modesInfo["id"].toInt();
        int width = modesInfo["width"].toInt();
        int height = modesInfo["width"].toInt();
        double refreshRate = modesInfo["refreshRate"].toDouble();

        if (ScreenWidth() == width && ScreenHeight() == height && refreshRate == dRefreshRate) {
            nModeId = Id;
            break;
        }
    }

    if (nModeId < 0)
        return;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                          xcb_randr_get_crtc_info(m_pXConnection, m_xcb_randr_crtc_t, 0), NULL);

    xcb_randr_output_t *poutput = xcb_randr_get_crtc_info_outputs(crtc);
    xcb_randr_set_crtc_config_cookie_t crtcConfigCookie = {};
    crtcConfigCookie = xcb_randr_set_crtc_config(m_pXConnection, m_xcb_randr_crtc_t, config_timestamp, config_timestamp, X(), Y(), nModeId, Rotation(), 1, poutput);

    free(crtc);
    xcb_flush(m_pXConnection);
}

void X11Screen::SetRotation(int nRotation)
{
    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                          xcb_randr_get_crtc_info(m_pXConnection, m_xcb_randr_crtc_t, 0), NULL);

    xcb_randr_output_t *poutput = xcb_randr_get_crtc_info_outputs(crtc);
    xcb_randr_set_crtc_config_cookie_t crtcConfigCookie = {};
    crtcConfigCookie = xcb_randr_set_crtc_config(m_pXConnection, m_xcb_randr_crtc_t, config_timestamp, config_timestamp, X(), Y(), crtc->mode, nRotation, 1, poutput);

    free(crtc);
    xcb_flush(m_pXConnection);
}

void X11Screen::SetColorTemperature(int nColorTemperature)
{
    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_crtc_gamma_cookie_t crtcGammaCookie = {};
    crtcGammaCookie = xcb_randr_get_crtc_gamma(m_pXConnection, m_xcb_randr_crtc_t);

    xcb_randr_get_crtc_gamma_reply_t *crtcGammaReply = {};
    crtcGammaReply = xcb_randr_get_crtc_gamma_reply(m_pXConnection, crtcGammaCookie, &xcbError);

    uint16_t *gammaRed = xcb_randr_get_crtc_gamma_red(crtcGammaReply);
    uint16_t *gammaGreen = xcb_randr_get_crtc_gamma_green(crtcGammaReply);
    uint16_t *gammaBlue = xcb_randr_get_crtc_gamma_blue(crtcGammaReply);

    xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie = xcb_randr_get_crtc_gamma_size(m_pXConnection, m_xcb_randr_crtc_t);
    xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply = xcb_randr_get_crtc_gamma_size_reply(m_pXConnection, gamma_size_cookie, &xcbError);

    unsigned int ramp_size = gamma_size_reply->size;

    colorrampFill(gammaRed, gammaGreen, gammaBlue, ramp_size, nColorTemperature, m_dBrightness);

    xcb_void_cookie_t gamma_set_cookie = xcb_randr_set_crtc_gamma(m_pXConnection, m_xcb_randr_crtc_t,
                                                                  ramp_size, gammaRed,
                                                                  gammaGreen, gammaBlue);

    free(crtcGammaReply);
    free(gamma_size_reply);
    xcb_flush(m_pXConnection);

    m_nColor = nColorTemperature;
}

void X11Screen::SetScreenPosition(int x, int y)
{
    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                          xcb_randr_get_crtc_info(m_pXConnection, m_xcb_randr_crtc_t, 0), NULL);

    xcb_randr_output_t *poutput = xcb_randr_get_crtc_info_outputs(crtc);
    xcb_randr_set_crtc_config_cookie_t crtcConfigCookie = {};
    crtcConfigCookie = xcb_randr_set_crtc_config(m_pXConnection, m_xcb_randr_crtc_t, config_timestamp, config_timestamp, x, y, crtc->mode, Rotation(), 1, poutput);

    free(crtc);
    xcb_flush(m_pXConnection);
}

void X11Screen::setScalingMode(int nMode)
{

}

void X11Screen::colorrampFill(uint16_t *gamma_r, uint16_t *gamma_g, uint16_t *gamma_b, int size, int temperature, double dBrightness)
{
    m_nColor = temperature;
    m_dBrightness = dBrightness;

    for (int i = 0; i < size; i++) {
        uint16_t value = (double)i/size * (UINT16_MAX+1);
        gamma_r[i] = value;
        gamma_g[i] = value;
        gamma_b[i] = value;
    }

    float whitePoint[3];
    float alpha = (temperature % 100) / 100.0;
    int bbCIndex = ((temperature - 1000) / 100) * 3;

    whitePoint[0] = (1. - alpha) * blackbodyColors[bbCIndex] + alpha * blackbodyColors[bbCIndex + 3];
    whitePoint[1] = (1. - alpha) * blackbodyColors[bbCIndex + 1] + alpha * blackbodyColors[bbCIndex + 4];
    whitePoint[2] = (1. - alpha) * blackbodyColors[bbCIndex + 2] + alpha * blackbodyColors[bbCIndex + 5];

    for (int i = 0; i < size; i++) {
        gamma_r[i] = (double)gamma_r[i] / (UINT16_MAX+1) * whitePoint[0] * (UINT16_MAX+1) * dBrightness;
        gamma_g[i] = (double)gamma_g[i] / (UINT16_MAX+1) * whitePoint[1] * (UINT16_MAX+1) * dBrightness;
        gamma_b[i] = (double)gamma_b[i] / (UINT16_MAX+1) * whitePoint[2] * (UINT16_MAX+1) * dBrightness;
    }
}

QString X11Screen::formatOutputName(QString strOutputName)
{
    QStringList strResultLst;
    QStringList outputname = strOutputName.split("-");
    for (int i = 0; i < outputname.count(); i++) {
        QString strName = outputname.at(i);
        if (strName.front().isNumber()) {
          strResultLst << strName.left(1);
          break;
        }
        strResultLst << strName;
    }
    return strResultLst.join("-");
}

QByteArray X11Screen::outputEdid(xcb_randr_output_t outputId)
{
    size_t len = 0;
    quint8 *result;

    auto edid_atom = XCB::InternAtom(false, 4, "EDID")->atom;
    result = X11Screen::getXProperty(outputId, edid_atom, len);
    if (result == nullptr) {
        auto edid_atom = XCB::InternAtom(false, 9, "EDID_DATA")->atom;
        result = X11Screen::getXProperty(outputId, edid_atom, len);
    }
    if (result == nullptr) {
        auto edid_atom = XCB::InternAtom(false, 25, "XFree86_DDC_EDID1_RAWDATA")->atom;
        result = X11Screen::getXProperty(outputId, edid_atom, len);
    }

    QByteArray edid;
    if (result != nullptr) {
        if (len % 128 == 0) {
            edid = QByteArray(reinterpret_cast<const char *>(result), len);
        }
        delete[] result;
    }
    return edid;
}

quint8 *X11Screen::getXProperty(xcb_randr_output_t output, xcb_atom_t atom, size_t &len)
{
    quint8 *result;

    auto cookie = xcb_randr_get_output_property(XCB::connection(), output, atom, XCB_ATOM_ANY, 0, 100, false, false);
    auto reply = xcb_randr_get_output_property_reply(XCB::connection(), cookie, nullptr);

    if (reply->type == XCB_ATOM_INTEGER && reply->format == 8) {
        result = new quint8[reply->num_items];
        memcpy(result, xcb_randr_get_output_property_data(reply), reply->num_items);
        len = reply->num_items;
    } else {
        result = nullptr;
    }

    free(reply);
    return result;
}

QByteArray X11Screen::extractEisaId(QByteArray edid)
{
    /*
     * From EDID standard section 3.4:
     * The ID Manufacturer Name field, shown in Table 3.5, contains a 2-byte representation of the monitor's
     * manufacturer. This is the same as the EISA ID. It is based on compressed ASCII, “0001=A” ... “11010=Z”.
     *
     * The table:
     * | Byte |        Bit                    |
     * |      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
     * ----------------------------------------
     * |  1   | 0)| (4| 3 | 2 | 1 | 0)| (4| 3 |
     * |      | * |    Character 1    | Char 2|
     * ----------------------------------------
     * |  2   | 2 | 1 | 0)| (4| 3 | 2 | 1 | 0)|
     * |      | Character2|      Character 3  |
     * ----------------------------------------
     **/
    const uint8_t *data = reinterpret_cast<uint8_t*>(edid.data());
    static const uint offset = 0x8;
    char id[4];
    if (data[offset] >> 7) {
        // bit at position 7 is not a 0
        return QByteArray();
    }
    // shift two bits to right, and with 7 right most bits
    id[0] = 'A' + ((data[offset] >> 2) & 0x1f) - 1;
    // for first byte: take last two bits and shift them 3 to left (000xx000)
    // for second byte: shift 5 bits to right and take 3 right most bits (00000xxx)
    // or both together
    id[1] = 'A' + (((data[offset] & 0x3) << 3) | ((data[offset + 1] >> 5) & 0x7)) - 1;
    // take five right most bits
    id[2] = 'A' + (data[offset + 1] & 0x1f) - 1;
    id[3] = '\0';
    return QByteArray(id);
}

QByteArray X11Screen::extractSerialNumber(QByteArray edid)
{
    // see section 3.4
    const uint8_t *data = reinterpret_cast<uint8_t*>(edid.data());
    static const uint offset = 0x0C;
    /*
     * The ID serial number is a 32-bit serial number used to differentiate between individual instances of the same model
     * of monitor. Its use is optional. When used, the bit order for this field follows that shown in Table 3.6. The EDID
     * structure Version 1 Revision 1 and later offer a way to represent the serial number of the monitor as an ASCII string
     * in a separate descriptor block.
     */
    uint32_t serialNumber = 0;
    serialNumber  = (uint32_t) data[offset + 0];
    serialNumber |= (uint32_t) data[offset + 1] << 8;
    serialNumber |= (uint32_t) data[offset + 2] << 16;
    serialNumber |= (uint32_t) data[offset + 3] << 24;
    if (serialNumber == 0) {
        return QByteArray();
    }
    return QByteArray::number(serialNumber);
}

QByteArray X11Screen::extractMonitorName(QByteArray edid)
{
    QByteArray monitorName;
    const uint8_t *data = reinterpret_cast<uint8_t*>(edid.data());
    static const uint offset = 0x36;
    static const uint blockLength = 18;
    for (int i = 0; i < 5; ++i) {
        const uint co = offset + i * blockLength;
        // Flag = 0000h when block used as descriptor
        if (data[co] != 0) {
            continue;
        }
        if (data[co + 1] != 0) {
            continue;
        }
        // Reserved = 00h when block used as descriptor
        if (data[co + 2] != 0) {
            continue;
        }
        /*
         * FFh: Monitor Serial Number - Stored as ASCII, code page # 437, ≤ 13 bytes.
         * FEh: ASCII String - Stored as ASCII, code page # 437, ≤ 13 bytes.
         * FDh: Monitor range limits, binary coded
         * FCh: Monitor name, stored as ASCII, code page # 437
         * FBh: Descriptor contains additional color point data
         * FAh: Descriptor contains additional Standard Timing Identifications
         * F9h - 11h: Currently undefined
         * 10h: Dummy descriptor, used to indicate that the descriptor space is unused
         * 0Fh - 00h: Descriptor defined by manufacturer.
         */
        if (data[co + 3] == 0xfc) {
            monitorName = QByteArray((const char *)(&data[co + 5]), 12).trimmed();
            break;
        }
    }
    return monitorName;
}

void X11Screen::getScalingModeInfo()
{
    Atom	    *props;
    int		    j, nprop;

    XRRScreenResources *res;
    XRROutputInfo *output_info = NULL;
    int i;
    int found = 0;
    QString strUuid;

    res = XRRGetScreenResources(m_pDisplay, DefaultRootWindow(m_pDisplay));

    for (i = 0; i < res->noutput && !found; i++) {
        output_info = XRRGetOutputInfo(m_pDisplay, res, res->outputs[i]);
        if (output_info->crtc && output_info->connection == RR_Connected && output_info->name == m_strScreenName) {
            props = XRRListOutputProperties(m_pDisplay, res->outputs[i], &nprop);

            for (j = 0; j < nprop; j++) {
                unsigned char *prop;
                int actual_format;
                unsigned long nitems, bytes_after;
                Atom actual_type;
                XRRPropertyInfo *propinfo;
                char *atom_name = XGetAtomName (m_pDisplay, props[j]);
                int k;
                XRRGetOutputProperty (m_pDisplay, res->outputs[i], props[j],
                                      0, 100, False, False,
                                      AnyPropertyType,
                                      &actual_type, &actual_format,
                                      &nitems, &bytes_after, &prop);

                propinfo = XRRQueryOutputProperty(m_pDisplay, res->outputs[i],
                                                  props[j]);

                if (QString::fromUtf8(atom_name) == SCALINGMODE) {
                    int bytes_per_item;
                    int m;
                    switch (actual_format) {
                    case 8:
                        bytes_per_item = sizeof(char);
                        break;
                    case 16:
                        bytes_per_item = sizeof(short);
                        break;
                    case 32:
                        bytes_per_item = sizeof(long);
                        break;
                    default:
                        break;
                    }

                    for (m = 0; m < nitems; m++) {
                        const void *value_bytes;
                        value_bytes = prop + (m * bytes_per_item);

                        if (actual_type == XA_ATOM && actual_format == 32)
                        {
                            const Atom *val = static_cast<const Atom *>(value_bytes);
                            char *str = XGetAtomName (m_pDisplay, *val);
                            if (str != NULL) {
                                currentScalingMode = str;
                                XFree (str);
                                break;
                            }
                        }
                    }

                    if (!propinfo->range && propinfo->num_values > 0) {
                        for (k = 0; k < propinfo->num_values; k++) {
                            const void *value_bytes = (unsigned char *) &(propinfo->values[k]);
                            if (actual_type == XA_ATOM) {
                                const Atom *val = static_cast<const Atom *>(value_bytes);
                                char *str = XGetAtomName(m_pDisplay, *val);
                                if (str != NULL) {
                                    m_supportScalingModeList << QString::fromUtf8(str);
                                    XFree(str);
                                }
                            }
                        }
                    }
                    free(propinfo);
                }
            }
        }
        XRRFreeOutputInfo(output_info);
    }
    XRRFreeScreenResources(res);
}
