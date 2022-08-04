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

#include "x11_output.h"

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
#include <QDir>
#include <QCryptographicHash>
#include <QCoreApplication>

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
#include <X11/extensions/XInput.h>
#include <X11/Xatom.h>

#include "xcbeventlistener.h"
#include "x11/x11_screen.h"

Display *m_pDisplay = nullptr;
xcb_connection_t* m_pXConnection = nullptr;
xcb_screen_t* m_pXFirstScreen = nullptr;
xcb_window_t m_XWindowDummy;
QString m_strTouchDevicePath;

Bool is_pointer(int use)
{
    return use == XIMasterPointer || use == XISlavePointer;
}

Bool is_keyboard(int use)
{
    return use == XIMasterKeyboard || use == XISlaveKeyboard;
}

Bool device_matches(XIDeviceInfo *info, char *name)
{
    if (strcmp(info->name, name) == 0) {
        return true;
    }

    if (strncmp(name, "pointer:", strlen("pointer:")) == 0 &&
        strcmp(info->name, name + strlen("pointer:")) == 0 &&
        is_pointer(info->use)) {
        return true;
    }

    if (strncmp(name, "keyboard:", strlen("keyboard:")) == 0 &&
        strcmp(info->name, name + strlen("keyboard:")) == 0 &&
        is_keyboard(info->use)) {
        return true;
    }

    return false;
}

XIDeviceInfo *xi2_find_device_info(char *name) {
    XIDeviceInfo *info;
    XIDeviceInfo *found = NULL;
    int ndevices;
    Bool is_id = true;
    int i, id = -1;

    for(i = 0; i < strlen(name); i++) {
        if (!isdigit(name[i])) {
            is_id = false;
            break;
        }
    }

    if (is_id) {
        id = atoi(name);
    }

    info = XIQueryDevice(m_pDisplay, XIAllDevices, &ndevices);
    for(i = 0; i < ndevices; i++)
    {
        if (is_id ? info[i].deviceid == id : device_matches (&info[i], name)) {
            if (found) {
                fprintf(stderr,
                        "Warning: There are multiple devices matching '%s'.\n"
                        "To ensure the correct one is selected, please use "
                        "the device ID, or prefix the\ndevice name with "
                        "'pointer:' or 'keyboard:' as appropriate.\n\n", name);
                XIFreeDeviceInfo(info);
                return NULL;
            } else {
                found = &info[i];
            }
        }
    }

    return found;
}

/* Caller must free return value */
static XRROutputInfo*
find_output_xrandr(Display *dpy, const char *output_name)
{
    XRRScreenResources *res;
    XRROutputInfo *output_info = NULL;
    int i;
    int found = 0;

    res = XRRGetScreenResources(dpy, DefaultRootWindow(dpy));

    for (i = 0; i < res->noutput && !found; i++)
    {
        output_info = XRRGetOutputInfo(dpy, res, res->outputs[i]);

        if (output_info->crtc && output_info->connection == RR_Connected &&
            strcmp(output_info->name, output_name) == 0)
        {
            found = 1;
            break;
        }

        XRRFreeOutputInfo(output_info);
    }

    XRRFreeScreenResources(res);

    if (!found)
        output_info = NULL;

    return output_info;
}

static void matrix_set(Matrix *m, int row, int col, float val)
{
    m->m[row * 3 + col] = val;
}

static void matrix_set_unity(Matrix *m)
{
    memset(m, 0, sizeof(m->m));
    matrix_set(m, 0, 0, 1);
    matrix_set(m, 1, 1, 1);
    matrix_set(m, 2, 2, 1);
}

static int
apply_matrix(Display *dpy, int deviceid, Matrix *m)
{
    Atom prop_float, prop_matrix;

    union {
        unsigned char *c;
        float *f;
    } data;
    int format_return;
    Atom type_return;
    unsigned long nitems;
    unsigned long bytes_after;

    int rc;

    prop_float = XInternAtom(dpy, "FLOAT", false);
    prop_matrix = XInternAtom(dpy, "Coordinate Transformation Matrix", false);

    if (!prop_float)
    {
        fprintf(stderr, "Float atom not found. This server is too old.\n");
        return EXIT_FAILURE;
    }
    if (!prop_matrix)
    {
        fprintf(stderr, "Coordinate transformation matrix not found. This "
                "server is too old\n");
        return EXIT_FAILURE;
    }

    rc = XIGetProperty(dpy, deviceid, prop_matrix, 0, 9, false, prop_float,
                       &type_return, &format_return, &nitems, &bytes_after,
                       &data.c);
    if (rc != Success || prop_float != type_return || format_return != 32 ||
        nitems != 9 || bytes_after != 0)
    {
        fprintf(stderr, "Failed to retrieve current property values\n");
        return EXIT_FAILURE;
    }

    memcpy(data.f, m->m, sizeof(m->m));

    XIChangeProperty(dpy, deviceid, prop_matrix, prop_float,
                     format_return, PropModeReplace, data.c, nitems);

    XFree(data.c);

    return EXIT_SUCCESS;
}

static void
matrix_s4(Matrix *m, float x02, float x12, float d1, float d2, int main_diag)
{
    matrix_set(m, 0, 2, x02);
    matrix_set(m, 1, 2, x12);

    if (main_diag) {
        matrix_set(m, 0, 0, d1);
        matrix_set(m, 1, 1, d2);
    } else {
        matrix_set(m, 0, 0, 0);
        matrix_set(m, 1, 1, 0);
        matrix_set(m, 0, 1, d1);
        matrix_set(m, 1, 0, d2);
    }
}

static void
set_transformation_matrix(Display *dpy, Matrix *m, int offset_x, int offset_y,
                          int screen_width, int screen_height,
                          int rotation)
{
    /* total display size */
    int width = DisplayWidth(dpy, DefaultScreen(dpy));
    int height = DisplayHeight(dpy, DefaultScreen(dpy));

    /* offset */
    float x = 1.0 * offset_x/width;
    float y = 1.0 * offset_y/height;

    /* mapping */
    float w = 1.0 * screen_width/width;
    float h = 1.0 * screen_height/height;

    matrix_set_unity(m);

    /*
     * There are 16 cases:
     * Rotation X Reflection
     * Rotation: 0 | 90 | 180 | 270
     * Reflection: None | X | Y | XY
     *
     * They are spelled out instead of doing matrix multiplication to avoid
     * any floating point errors.
     */
    switch (rotation) {
    case RR_Rotate_0:
    case RR_Rotate_180 | RR_Reflect_All:
        matrix_s4(m, x, y, w, h, 1);
        break;
    case RR_Reflect_X|RR_Rotate_0:
    case RR_Reflect_Y|RR_Rotate_180:
        matrix_s4(m, x + w, y, -w, h, 1);
        break;
    case RR_Reflect_Y|RR_Rotate_0:
    case RR_Reflect_X|RR_Rotate_180:
        matrix_s4(m, x, y + h, w, -h, 1);
        break;
    case RR_Rotate_90:
    case RR_Rotate_270 | RR_Reflect_All: /* left limited - correct in working zone. */
        matrix_s4(m, x + w, y, -w, h, 0);
        break;
    case RR_Rotate_270:
    case RR_Rotate_90 | RR_Reflect_All: /* left limited - correct in working zone. */
        matrix_s4(m, x, y + h, w, -h, 0);
        break;
    case RR_Rotate_90 | RR_Reflect_X: /* left limited - correct in working zone. */
    case RR_Rotate_270 | RR_Reflect_Y: /* left limited - correct in working zone. */
        matrix_s4(m, x, y, w, h, 0);
        break;
    case RR_Rotate_90 | RR_Reflect_Y: /* right limited - correct in working zone. */
    case RR_Rotate_270 | RR_Reflect_X: /* right limited - correct in working zone. */
        matrix_s4(m, x + w, y + h, -w, -h, 0);
        break;
    case RR_Rotate_180:
    case RR_Reflect_All|RR_Rotate_0:
        matrix_s4(m, x + w, y + h, -w, -h, 1);
        break;
    }
}

static int
map_output_xrandr(Display *dpy, int deviceid, const char *output_name)
{
    int rc = EXIT_FAILURE;
    XRRScreenResources *res;
    XRROutputInfo *output_info;

    res = XRRGetScreenResources(dpy, DefaultRootWindow(dpy));
    output_info = find_output_xrandr(dpy, output_name);

    /* crtc holds our screen info, need to compare to actual screen size */
    if (output_info)
    {
        XRRCrtcInfo *crtc_info;
        Matrix m;
        matrix_set_unity(&m);
        crtc_info = XRRGetCrtcInfo (dpy, res, output_info->crtc);
        set_transformation_matrix(dpy, &m, crtc_info->x, crtc_info->y,
                                  crtc_info->width, crtc_info->height,
                                  crtc_info->rotation);
        rc = apply_matrix(dpy, deviceid, &m);
        XRRFreeCrtcInfo(crtc_info);
        XRRFreeOutputInfo(output_info);
    } else
        printf("Unable to find output '%s'. "
                "Output may not be connected.\n", output_name);

    XRRFreeScreenResources(res);

    return rc;
}

static int
map_output_xinerama(Display *dpy, int deviceid, const char *output_name)
{
    const char *prefix = "HEAD-";
    XineramaScreenInfo *screens = NULL;
    int rc = EXIT_FAILURE;
    int event, error;
    int nscreens;
    int head;
    Matrix m;

    if (!XineramaQueryExtension(dpy, &event, &error))
    {
        fprintf(stderr, "Unable to set screen mapping. Xinerama extension not found\n");
        goto out;
    }

    if (strlen(output_name) < strlen(prefix) + 1 ||
        strncmp(output_name, prefix, strlen(prefix)) != 0)
    {
        fprintf(stderr, "Please specify the output name as HEAD-X,"
                "where X is the screen number\n");
        goto out;
    }

    head = output_name[strlen(prefix)] - '0';

    screens = XineramaQueryScreens(dpy, &nscreens);

    if (nscreens == 0)
    {
        fprintf(stderr, "Xinerama failed to query screens.\n");
        goto out;
    } else if (nscreens <= head)
    {
        fprintf(stderr, "Found %d screens, but you requested %s.\n",
                nscreens, output_name);
        goto out;
    }

    matrix_set_unity(&m);
    set_transformation_matrix(dpy, &m,
                              screens[head].x_org, screens[head].y_org,
                              screens[head].width, screens[head].height,
                              RR_Rotate_0);
    rc = apply_matrix(dpy, deviceid, &m);

out:
    XFree(screens);
    return rc;
}

X11Output::X11Output(QObject *parent)
    : AbstractOutput(parent)
{
    m_pDisplay = XOpenDisplay(nullptr);
    m_pXConnection = xcb_connect(0, 0);
    m_pXFirstScreen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    m_XWindowDummy = xcb_generate_id(m_pXConnection);

    xcb_create_window(m_pXConnection, 0, m_XWindowDummy, m_pXFirstScreen->root,
                      0, 0, 1, 1, 0, 0, 0, 0, 0);

    xcb_flush(m_pXConnection);

    m_x11Helper = new XCBEventListener(m_pXConnection, m_pXFirstScreen, m_XWindowDummy);
    connect(m_x11Helper, &XCBEventListener::outputChanged, this, &X11Output::outputChanged);
    connect(m_x11Helper, &XCBEventListener::crtcChanged, this, &X11Output::crtcChanged);
    connect(m_x11Helper, &XCBEventListener::screenChanged, this, &X11Output::screenChanged);

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        xcb_randr_crtc_t pxcb_randr_crtc_t = outputInfoReply->crtc;
        char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);

        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(formatOutputName(QString::fromUtf8(monitorName)).toStdString().c_str());
        QString strName = QString::fromStdString(hash.result().toHex().left(8).toStdString());

        X11Screen *pX11Screen = new X11Screen(m_pXConnection, m_pXFirstScreen, m_XWindowDummy, strName, pxcb_randr_crtc_t, m_x11Helper);
        pX11Screen->setObjectName(strName);

        free(outputInfoReply);
        free(crtc);
    }
    free(reply);
    xcb_flush(m_pXConnection);
}

X11Output:: ~X11Output()
{
    XCloseDisplay(m_pDisplay);
    xcb_disconnect(m_pXConnection);
}

QVariantList X11Output::Touchscreens()
{
    QVariantList touchScreenList;
    int devid = 0;
    XIDeviceInfo *di;
    XIDeviceInfo *devInfo;
    XITouchClassInfo *a;
    int cnt;
    int i, j;
    int         nprops;
    Atom        *props;
    XDevice     *dev;
    XIAnyClassInfo *pointerClassInfo;

    di = XIQueryDevice(m_pDisplay, XIAllDevices, &cnt);

    for (i = 0; i < cnt; i ++) {
        devInfo = &di[i];
        for (j = 0; j < devInfo->num_classes; j ++) {
            a = (XITouchClassInfo*)(devInfo->classes[j]);
            if (a->type == XITouchClass) {
                devid = devInfo->deviceid;

                dev = XOpenDevice(m_pDisplay, devid);

                if (!dev) {
                    break;
                }

                props = XListDeviceProperties(m_pDisplay, dev, &nprops);
                while (nprops--) {
                    Atom                act_type;
                    char                *name;
                    int                 act_format;
                    unsigned long       nitems, bytes_after;
                    unsigned char       *data;
                    int                 j, done = False, size = 0;

                    if (XGetDeviceProperty(m_pDisplay, dev, props[nprops], 0, 1000, False,
                                           AnyPropertyType, &act_type, &act_format,
                                           &nitems, &bytes_after, &data) == Success) {
                        switch (act_format) {
                        case 8: size = sizeof(char); break;
                        case 16: size = sizeof(short); break;
                        case 32: size = sizeof(long); break;
                        }

                        for (j = 0; j < nitems; j++) {
                            if (act_type == XA_STRING) {
                                if (act_format == 8) {
                                    m_strTouchDevicePath = QString::fromUtf8((char *)data);
                                    break;
                                }
                            }
                        }
                    }
                }

                QMap<QString, QVariant> touchInfo;

                XRRScreenResources *res;
                XRROutputInfo *output_info = NULL;
                int i;
                int found = 0;
                QString strUuid;

                res = XRRGetScreenResources(m_pDisplay, DefaultRootWindow(m_pDisplay));

                for (i = 0; i < res->noutput && !found; i++) {
                    output_info = XRRGetOutputInfo(m_pDisplay, res, res->outputs[i]);
                    if (output_info->crtc && output_info->connection == RR_Connected) {
                        QCryptographicHash hash(QCryptographicHash::Md5);
                        hash.addData(formatOutputName(QString::fromUtf8(output_info->name)).toStdString().c_str());
                        strUuid = QString::fromStdString(hash.result().toHex().left(8).toStdString());
                        break;
                    }
                    XRRFreeOutputInfo(output_info);
                }
                XRRFreeScreenResources(res);

                touchInfo["TouchDevice"] = m_strTouchDevicePath;
                touchInfo["ScreenUuid"] = strUuid;
                touchInfo["ScreenName"] = devInfo->name;
                touchScreenList << touchInfo;
            }
        }
    }
    XIFreeDeviceInfo(di);
    return touchScreenList;
}

QString X11Output::PrimaryScreenName()
{
    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_output_primary_cookie_t outputPrimaryCookie = {};
    outputPrimaryCookie = xcb_randr_get_output_primary(m_pXConnection, m_XWindowDummy);

    xcb_randr_get_output_primary_reply_t *outputPrimaryReply = {};
    outputPrimaryReply = xcb_randr_get_output_primary_reply(m_pXConnection, outputPrimaryCookie, &xcbError);

    xcb_randr_get_output_info_cookie_t output_info_cookie = {};
    output_info_cookie = xcb_randr_get_output_info(m_pXConnection, outputPrimaryReply->output, config_timestamp);

    xcb_randr_get_output_info_reply_t *outputInfoReply = {};
    outputInfoReply = xcb_randr_get_output_info_reply(m_pXConnection, output_info_cookie, &xcbError);

    char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);

    free(outputPrimaryReply);
    free(outputInfoReply);

    return formatOutputName(QString::fromUtf8(monitorName));
}

QStringList X11Output::Monitors()
{
    QStringList strMonitorList;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        xcb_randr_crtc_t pxcb_randr_crtc_t = outputInfoReply->crtc;
        char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);

        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(formatOutputName(QString::fromUtf8(monitorName)).toStdString().c_str());
        strMonitorList << QString("%1%2").arg("/com/deepin/kwin/Display/Screen_").arg(QString::fromStdString(hash.result().toHex().left(8).toStdString()));

        free(outputInfoReply);
        free(crtc);
    }
    free(reply);
    xcb_flush(m_pXConnection);

    return strMonitorList;
}

uint X11Output::MaxBacklightBrightness()
{
    QString strBacklight = "/sys/class/backlight";
    QDir dir(strBacklight);
    if (!dir.exists()) {
        return uint();
    }

    QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < names.count(); i++) {
        QString str = QString(strBacklight).append(names.at(i)).append("/max_brightness");
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

ushort (X11Output::DisplayWidth)()
{
    int displayWidth = 0;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        if (crtc->x + crtc->width > displayWidth) {
            displayWidth = crtc->x + crtc->width;
        }
        free(outputInfoReply);
        free(crtc);
    }
    free(reply);
    xcb_flush(m_pXConnection);

    return displayWidth;
}

ushort (X11Output::DisplayHeight)()
{
    int displayHeight = 0;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        if (crtc->y + crtc->height > displayHeight) {
            displayHeight = crtc->y + crtc->height;
        }
        free(outputInfoReply);
        free(crtc);
    }
    free(reply);
    xcb_flush(m_pXConnection);

    return displayHeight;
}

QMap<QString, QVariant> X11Output::TouchMap()
{
    return touchScreensInfo;
}

int X11Output::ColorTemperatureMode()
{
    return m_nCCTMode;
}

uchar X11Output::DisplayMode()
{
    QList<xcb_randr_get_crtc_info_reply_t *> crtcList;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        crtcList << crtc;

        free(outputInfoReply);
        free(crtc);

    }
    free(reply);

    if (nDisplayMode == 0) {
        if (crtcList.count() > 1) {
            for (int i = 0 ; i < crtcList.count(); i++) {
                xcb_randr_get_crtc_info_reply_t *crtc = crtcList.at(i);
                if (crtc->x > 0 || crtc->y > 0) {
                    return screenExtendMode;
                } else {
                    return screenCopyMode;
                }
            }
        } else {
            return screenShowAloneMode;
        }

    }
    return nDisplayMode;
}

void X11Output::SetPrimaryScreen(QString strOutputName)
{
    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        xcb_randr_crtc_t pxcb_randr_crtc_t = outputInfoReply->crtc;
        char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);
        if (formatOutputName(QString::fromUtf8(monitorName)) == strOutputName) {
            xcb_randr_set_output_primary(m_pXConnection, m_XWindowDummy, randr_outputs[i]);
            xcb_flush(m_pXConnection);
            return;
        }
        free(outputInfoReply);
        free(crtc);
    }
    free(reply);
    xcb_flush(m_pXConnection);
}

void X11Output::showCursor()
{
    if (m_pDisplay == nullptr) {
        sendErrorReply(QDBusError::ErrorType::Failed, QString("showCursor: can't open display!"));
        return;
    }
    XFixesShowCursor(m_pDisplay, DefaultRootWindow(m_pDisplay));
    XFlush(m_pDisplay);
}

void X11Output::hideCursor()
{
    if (m_pDisplay == nullptr) {
        sendErrorReply(QDBusError::ErrorType::Failed, QString("hideCursor: can't open display!"));
        return;
    }
    XFixesHideCursor(m_pDisplay, DefaultRootWindow(m_pDisplay));
    XFlush(m_pDisplay);
}

void X11Output::SetDisplayMode(uchar uMode, QVariantList screenInfoList)
{
    /*
     * set Display mode need every output info.
     * we according to outputinfoList to set display info.
     * multiple outputs info please use multiple QMap to save.
     * This is an example of data to be set.
     *
     * QVariantList list;
     * QMap<QString, QVariant> screenInfo1;
     * screenInfo1["name"] = "VGA-0";
     * screenInfo1["uuid"] = "";
     * screenInfo1["enable"] = true;
     * screenInfo1["rotation"] = XCB_RANDR_ROTATION_ROTATE_0;
     * screenInfo1["x"] = 0;
     * screenInfo1["y"] = 0;
     * screenInfo1["modeId"] = 84;
     * screenInfo1["primary"] = true;
     * screenInfo1["brightness"] = 0.8;
     *
     * list << screenInfo1;
     **/

    nDisplayMode = uMode;

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

        xcb_generic_error_t *xcbError;
        xcb_timestamp_t     config_timestamp;

        xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                    m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

        xcb_timestamp_t timestamp = reply->config_timestamp;
        int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
        xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);

        for (int j = 0; j < len; j++) {
            xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                        m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[j], timestamp), NULL);
            if (outputInfoReply == NULL)
                continue;

            if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
                continue;

            xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                                  xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

            xcb_randr_crtc_t pxcb_randr_crtc_t = outputInfoReply->crtc;

            char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);

            if (formatOutputName(QString::fromUtf8(monitorName)) == strName) {

                if (bPrimary) {
                    xcb_randr_set_output_primary(m_pXConnection, m_XWindowDummy, randr_outputs[j]);
                }

                xcb_randr_output_t *poutput = xcb_randr_get_crtc_info_outputs(crtc);

                xcb_randr_set_crtc_config_cookie_t crtcConfigCookie = {};

                if (bEnable) {
                    crtcConfigCookie = xcb_randr_set_crtc_config(m_pXConnection, pxcb_randr_crtc_t, config_timestamp, config_timestamp, x, y, nMode, nRotation, 1, poutput);
                } else {
                    crtcConfigCookie = xcb_randr_set_crtc_config(m_pXConnection, pxcb_randr_crtc_t, config_timestamp, config_timestamp, 0, 0, XCB_NONE, XCB_RANDR_ROTATION_ROTATE_0, 0, nullptr);
                    break;
                }

                xcb_randr_get_crtc_gamma_cookie_t crtcGammaCookie = {};
                crtcGammaCookie = xcb_randr_get_crtc_gamma(m_pXConnection, pxcb_randr_crtc_t);

                xcb_randr_get_crtc_gamma_reply_t *crtcGammaReply = {};
                crtcGammaReply = xcb_randr_get_crtc_gamma_reply(m_pXConnection, crtcGammaCookie, &xcbError);

                uint16_t *gammaRed = xcb_randr_get_crtc_gamma_red(crtcGammaReply);
                uint16_t *gammaGreen = xcb_randr_get_crtc_gamma_green(crtcGammaReply);
                uint16_t *gammaBlue = xcb_randr_get_crtc_gamma_blue(crtcGammaReply);

                xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie = xcb_randr_get_crtc_gamma_size(m_pXConnection, pxcb_randr_crtc_t);
                xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply = xcb_randr_get_crtc_gamma_size_reply(m_pXConnection, gamma_size_cookie, &xcbError);

                unsigned int ramp_size = gamma_size_reply->size;

                colorramp_fill(gammaRed, gammaGreen, gammaBlue, ramp_size, m_nColor, dBrightness);

                xcb_void_cookie_t gamma_set_cookie = xcb_randr_set_crtc_gamma(m_pXConnection, pxcb_randr_crtc_t,
                                                                              ramp_size, gammaRed,
                                                                              gammaGreen, gammaBlue);

                xcb_randr_set_crtc_config_reply_t *crtcConfigReply = xcb_randr_set_crtc_config_reply(m_pXConnection, crtcConfigCookie, nullptr);

                if (!crtcConfigReply) {
                    sendErrorReply(QDBusError::ErrorType::Failed, "Error: Result: set crtc config error!");
                }
                free(crtcConfigReply);
                free(crtcGammaReply);
                free(gamma_size_reply);
            }
            free(outputInfoReply);
            free(crtc);
        }
        free(reply);
    }

    int nWidth = (DisplayWidth)();
    int nHeight = (DisplayHeight)();

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    const double dpi = 25.4 * screen->height_in_pixels / screen->height_in_millimeters;
    const int widthMM = (25.4 * nWidth) / dpi;
    const int heightMM = (25.4 * nHeight) / dpi;
    xcb_randr_set_screen_size(m_pXConnection, m_XWindowDummy, nWidth, nHeight, widthMM, heightMM);

    xcb_flush(m_pXConnection);
}

void X11Output::SetColorTemperatureMode(int nValue)
{
     m_nCCTMode = nValue;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        xcb_randr_crtc_t pxcb_randr_crtc_t = outputInfoReply->crtc;

        xcb_randr_get_crtc_gamma_cookie_t crtcGammaCookie = {};
        crtcGammaCookie = xcb_randr_get_crtc_gamma(m_pXConnection, pxcb_randr_crtc_t);

        xcb_randr_get_crtc_gamma_reply_t *crtcGammaReply = {};
        crtcGammaReply = xcb_randr_get_crtc_gamma_reply(m_pXConnection, crtcGammaCookie, &xcbError);

        uint16_t *gammaRed = xcb_randr_get_crtc_gamma_red(crtcGammaReply);
        uint16_t *gammaGreen = xcb_randr_get_crtc_gamma_green(crtcGammaReply);
        uint16_t *gammaBlue = xcb_randr_get_crtc_gamma_blue(crtcGammaReply);

        xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie = xcb_randr_get_crtc_gamma_size(m_pXConnection, pxcb_randr_crtc_t);
        xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply = xcb_randr_get_crtc_gamma_size_reply(m_pXConnection, gamma_size_cookie, &xcbError);

        char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);
        unsigned int ramp_size = gamma_size_reply->size;

        colorramp_fill(gammaRed, gammaGreen, gammaBlue, ramp_size, nValue, m_dBrightness);

        xcb_void_cookie_t gamma_set_cookie = xcb_randr_set_crtc_gamma(m_pXConnection, pxcb_randr_crtc_t,
                                                                      ramp_size, gammaRed,
                                                                      gammaGreen, gammaBlue);
        free(outputInfoReply);
        free(crtc);
        free(crtcGammaReply);
        free(gamma_size_reply);
    }
    free(reply);
    xcb_flush(m_pXConnection);
}

double X11Output::Scale()
{
    double dScale = 1.00;

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    xcb_randr_get_screen_resources_reply_t *reply =
            xcb_randr_get_screen_resources_reply(m_pXConnection,
                                                 xcb_randr_get_screen_resources(m_pXConnection, screen->root),
                                                 NULL);

    xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_modes(reply);

    xcb_randr_get_screen_resources_current_reply_t *reply1 = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply1);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply1);

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_transform_cookie_t transformCookie = {};
        transformCookie = xcb_randr_get_crtc_transform(m_pXConnection, outputInfoReply->crtc);

        xcb_randr_get_crtc_transform_reply_t *transformReply = xcb_randr_get_crtc_transform_reply(m_pXConnection, transformCookie, NULL);

        if (!transformReply) {
            qDebug() << "transformReply is null.";
        }

        xcb_render_transform_t renderTransform = transformReply->current_transform;

        dScale = XFixedToDouble(renderTransform.matrix11);

        free(transformReply);
        free(outputInfoReply);
    }

    free(reply);

    if (dScale < 1) {
        dScale = ceil(dScale) + qAbs(ceil(dScale) - dScale);
    } else {
        dScale = int(dScale) - qAbs(int(dScale) - dScale);
    }

    return dScale;
}

void X11Output::SetScale(double dScale)
{
    double scale = 1.00;
    if (dScale < 1) {
        scale = ceil(dScale) + qAbs(ceil(dScale) - dScale);
    } else {
        scale = int(dScale) - qAbs(int(dScale) - dScale);
    }

    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    xcb_randr_get_screen_resources_reply_t *reply =
            xcb_randr_get_screen_resources_reply(m_pXConnection,
                                                 xcb_randr_get_screen_resources(m_pXConnection, screen->root),
                                                 NULL);

    xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_modes(reply);

    xcb_randr_get_screen_resources_current_reply_t *reply1 = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply1);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply1);

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        xcb_randr_crtc_t pxcb_randr_crtc_t = outputInfoReply->crtc;

        xcb_randr_output_t *poutput = xcb_randr_get_crtc_info_outputs(crtc);

        xcb_randr_get_crtc_transform_cookie_t transformCookie = {};
        transformCookie = xcb_randr_get_crtc_transform(m_pXConnection, outputInfoReply->crtc);

        xcb_randr_get_crtc_transform_reply_t *transformReply = xcb_randr_get_crtc_transform_reply(m_pXConnection, transformCookie, NULL);

        if (!transformReply) {
            qDebug() << "transformReply is null.";
        }

        xcb_render_transform_t renderTransform = transformReply->current_transform;

        renderTransform.matrix11 = XDoubleToFixed (scale);
        renderTransform.matrix12 = 0;
        renderTransform.matrix13 = 0;
        renderTransform.matrix21 = 0;
        renderTransform.matrix22 = XDoubleToFixed (scale);
        renderTransform.matrix23 = 0;
        renderTransform.matrix31 = 0;
        renderTransform.matrix32 = 0;

        const char *filter = scale == 1.0 ? "nearest" : "bilinear";

        xcb_void_cookie_t cookie = xcb_randr_set_crtc_transform(m_pXConnection, outputInfoReply->crtc, renderTransform, qstrlen(filter),
                                                                filter, 0,
                                                                nullptr);

        xcb_randr_set_crtc_config(m_pXConnection, pxcb_randr_crtc_t, config_timestamp, config_timestamp, double(crtc->x) / m_dScale  * scale, double(crtc->y) / m_dScale * scale, crtc->mode, crtc->rotation, 1, poutput);

        free(transformReply);
        free(outputInfoReply);
    }

    free(reply);

    int nWidth = (DisplayWidth)();
    int nHeight = (DisplayHeight)();

    const double dpi = 25.4 * screen->height_in_pixels / screen->height_in_millimeters;
    const int widthMM = (25.4 * nWidth) / dpi;
    const int heightMM = (25.4 * nHeight) / dpi;
    xcb_randr_set_screen_size(m_pXConnection, m_XWindowDummy, nWidth, nHeight, widthMM, heightMM);
    xcb_flush(m_pXConnection);

    m_dScale = scale;
}

void X11Output::setBrightness(double dBrightness)
{
    xcb_generic_error_t *xcbError;
    xcb_timestamp_t     config_timestamp;

    xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        xcb_randr_crtc_t pxcb_randr_crtc_t = outputInfoReply->crtc;

        xcb_randr_get_crtc_gamma_cookie_t crtcGammaCookie = {};
        crtcGammaCookie = xcb_randr_get_crtc_gamma(m_pXConnection, pxcb_randr_crtc_t);

        xcb_randr_get_crtc_gamma_reply_t *crtcGammaReply = {};
        crtcGammaReply = xcb_randr_get_crtc_gamma_reply(m_pXConnection, crtcGammaCookie, &xcbError);

        uint16_t *gammaRed = xcb_randr_get_crtc_gamma_red(crtcGammaReply);
        uint16_t *gammaGreen = xcb_randr_get_crtc_gamma_green(crtcGammaReply);
        uint16_t *gammaBlue = xcb_randr_get_crtc_gamma_blue(crtcGammaReply);

        xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie = xcb_randr_get_crtc_gamma_size(m_pXConnection, pxcb_randr_crtc_t);
        xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply = xcb_randr_get_crtc_gamma_size_reply(m_pXConnection, gamma_size_cookie, &xcbError);

        char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);
        unsigned int ramp_size = gamma_size_reply->size;

        colorramp_fill(gammaRed, gammaGreen, gammaBlue, ramp_size, m_nColor, dBrightness);

        xcb_void_cookie_t gamma_set_cookie = xcb_randr_set_crtc_gamma(m_pXConnection, pxcb_randr_crtc_t,
                                                                      ramp_size, gammaRed,
                                                                      gammaGreen, gammaBlue);
        free(outputInfoReply);
        free(crtc);
        free(crtcGammaReply);
        free(gamma_size_reply);
    }
    free(reply);
    xcb_flush(m_pXConnection);
}

void X11Output::mapToOutput(int nDeviceId, QString strOutputName)
{
    XIDeviceInfo *info;
    XRROutputInfo *output_info;

    std::string str = strOutputName.toStdString();
    const char *output_name = str.c_str();
    char *deviceId = (QString("%1").arg(nDeviceId)).toLatin1().data();

    info = xi2_find_device_info(deviceId);
    if (!info) {
        fprintf(stderr, "unable to find device '%s'\n", deviceId);
        return ;
    }

    output_info = find_output_xrandr(m_pDisplay, output_name);
    if (!output_info) {
        /* Output doesn't exist. Is this a (partial) non-RandR setup?  */
        output_info = find_output_xrandr(m_pDisplay, "default");
        if (output_info) {
            XRRFreeOutputInfo(output_info);
            if (strncmp("HEAD-", output_name, strlen("HEAD-")) == 0) {
                map_output_xinerama(m_pDisplay, info->deviceid, output_name);
                return;
            }

        }
    } else {
        XRRFreeOutputInfo(output_info);
    }

    map_output_xrandr(m_pDisplay, info->deviceid, output_name);
    XFlush(m_pDisplay);

    touchScreensInfo[QString::fromUtf8(info->name)] = strOutputName;
    return;
}

QString X11Output::formatOutputName(QString strOutputName)
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

void X11Output::colorramp_fill(uint16_t *gamma_r, uint16_t *gamma_g, uint16_t *gamma_b, int size, int temperature, double dBrightness)
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

void X11Output::testGetMode()
{
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pXConnection)).data;
    xcb_randr_get_screen_resources_reply_t *reply =
            xcb_randr_get_screen_resources_reply(m_pXConnection,
                                                 xcb_randr_get_screen_resources(m_pXConnection, screen->root),
                                                 NULL);

    xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_modes(reply);

    xcb_randr_get_screen_resources_current_reply_t *reply1 = xcb_randr_get_screen_resources_current_reply(
                m_pXConnection, xcb_randr_get_screen_resources_current(m_pXConnection, m_XWindowDummy), NULL);

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply1);
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply1);

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *outputInfoReply = xcb_randr_get_output_info_reply(
                    m_pXConnection, xcb_randr_get_output_info(m_pXConnection, randr_outputs[i], timestamp), NULL);
        if (outputInfoReply == NULL)
            continue;

        if (outputInfoReply->crtc == XCB_NONE || outputInfoReply->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            continue;

        xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(m_pXConnection,
                                                                              xcb_randr_get_crtc_info(m_pXConnection, outputInfoReply->crtc, timestamp), NULL);

        xcb_randr_mode_t *outputModes = xcb_randr_get_output_info_modes(outputInfoReply);

        char* monitorName = (char *)xcb_randr_get_output_info_name(outputInfoReply);

        for (int i = 0; i < outputInfoReply->num_modes; i++) {
             for (int j = 0; j < reply->num_modes; j++) {
                 if (modes[j].id != outputModes[i]) {
                     continue;
                 }
                 qDebug() << formatOutputName(QString::fromUtf8(monitorName)) << modes[j].id << modes[j].width << modes[j].height << ((double)modes[j].dot_clock / (double)(modes[j].htotal * modes[j].vtotal));
             }
        }
    }
}

void X11Output::outputChanged(xcb_randr_output_t output, xcb_randr_crtc_t crtc, xcb_randr_mode_t mode, xcb_randr_connection_t connection)
{
    if (crtc == XCB_NONE && mode == XCB_NONE && connection == XCB_RANDR_CONNECTION_DISCONNECTED) {
        XCB::OutputInfo info(output, XCB_TIME_CURRENT_TIME);
        if (!info.isNull()) {
            char* monitorName = (char *)xcb_randr_get_output_info_name(info);
            QString strName = formatOutputName(QString::fromUtf8(monitorName));

            QCryptographicHash hash(QCryptographicHash::Md5);
            hash.addData(formatOutputName(QString(strName)).toStdString().c_str());
            QString strUuid = QString::fromStdString(hash.result().toHex().left(8).toStdString());

            QString strScreenPath = QString("%1%2").arg("/com/deepin/kwin/Display/Screen_").arg(strUuid);
            QDBusConnection::sessionBus().unregisterObject(strScreenPath, QDBusConnection::UnregisterTree);

            QMap<QString, QVariant> OutputDeviceInfo;
            OutputDeviceInfo["screenName"] = strName;
            OutputDeviceInfo["uuid"] = strUuid;

            X11Screen *pX11Screen = m_x11Helper->findChild<X11Screen *>(strUuid);

            if (pX11Screen) {
                pX11Screen->setParent(nullptr);
                delete pX11Screen;
                pX11Screen = nullptr;
            }

            qCDebug(DEEPIN_DISPLAY_OUTPUT) << "Output: "<< strName << " ----- " << strUuid << " removed";

            emit outputRemoved(OutputDeviceInfo);
            return;
        }
    } else if (connection == XCB_RANDR_CONNECTION_CONNECTED) {
        XCB::OutputInfo info(output, XCB_TIME_CURRENT_TIME);
        if (!info.isNull()) {
            char* monitorName = (char *)xcb_randr_get_output_info_name(info);
            QString strName = formatOutputName(QString::fromUtf8(monitorName));

            QCryptographicHash hash(QCryptographicHash::Md5);
            hash.addData(strName.toStdString().c_str());
            QString strUuid = QString::fromStdString(hash.result().toHex().left(8).toStdString());

            QMap<QString, QVariant> OutputDeviceInfo;
            OutputDeviceInfo["screenName"] = strName;
            OutputDeviceInfo["uuid"] = strUuid;

            X11Screen *pX11Screen = m_x11Helper->findChild<X11Screen *>(strUuid);
            if (!pX11Screen) {
                pX11Screen = new X11Screen(m_pXConnection, m_pXFirstScreen, m_XWindowDummy, strUuid, crtc, m_x11Helper);
                pX11Screen->setObjectName(strUuid);
            }
            qCDebug(DEEPIN_DISPLAY_OUTPUT) << "Output" << strName << " ----- " << strUuid << " added";

            emit outputAdded();
            return;
        }
    }
}

void X11Output::crtcChanged(xcb_randr_crtc_t crtc, xcb_randr_mode_t mode, xcb_randr_rotation_t rotation, const QRect &geom, xcb_timestamp_t timestamp)
{
     qCDebug(DEEPIN_DISPLAY_OUTPUT) << "crtcChanged";
}

void X11Output::screenChanged(xcb_randr_rotation_t rotation, const QSize &sizePx, const QSize &sizeMm)
{
     qCDebug(DEEPIN_DISPLAY_OUTPUT) << "screenChanged";
}
