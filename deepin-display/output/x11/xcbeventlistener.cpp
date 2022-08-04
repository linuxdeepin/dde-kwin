/*
 *  SPDX-FileCopyrightText: 2012, 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "xcbeventlistener.h"

#include <QGuiApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

Q_LOGGING_CATEGORY(DEEPIN_DISPLAY_OUTPUT, "X11Output.xcb.helper")

XCBEventListener::XCBEventListener(xcb_connection_t *pXConnection, xcb_screen_t *pXFirstScreen, xcb_window_t XWindowDummy)
    : m_isRandrPresent(false)
    , m_randrBase(0)
    , m_randrErrorBase(0)
    , m_majorOpcode(0)
    , m_versionMajor(0)
    , m_versionMinor(0)
    , m_window(0)
{
    m_window = XWindowDummy;
    xcb_prefetch_extension_data(pXConnection, &xcb_randr_id);
    auto cookie = xcb_randr_query_version(pXConnection, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
    const auto *queryExtension = xcb_get_extension_data(pXConnection, &xcb_randr_id);
    if (!queryExtension) {
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "Fail to query for xrandr extension";
        return;
    }
    if (!queryExtension->present) {
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "XRandR extension is not present at all";
        return;
    }

    m_isRandrPresent = queryExtension->present;
    m_randrBase = queryExtension->first_event;
    m_randrErrorBase = queryExtension->first_error;
    m_majorOpcode = queryExtension->major_opcode;

    xcb_generic_error_t *error = nullptr;
    auto *versionReply = xcb_randr_query_version_reply(pXConnection, cookie, &error);
    Q_ASSERT_X(versionReply, "xrandrxcbhelper", "Query to fetch xrandr version failed");
    if (error) {
        qFatal("Error while querying for xrandr version: %d", error->error_code);
    }
    m_versionMajor = versionReply->major_version;
    m_versionMinor = versionReply->minor_version;
    free(versionReply);

//    qCDebug(DEEPIN_DISPLAY_OUTPUT).nospace() << "Detected XRandR " << m_versionMajor << "." << m_versionMinor;
//    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "Event Base: " << m_randrBase;
//    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "Event Error: " << m_randrErrorBase;

    xcb_randr_select_input(pXConnection,
                           m_window,
                           XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE | XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE
                               | XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

    qApp->installNativeEventFilter(this);
}

XCBEventListener::~XCBEventListener()
{
    if (m_window && QX11Info::connection()) {
        xcb_destroy_window(QX11Info::connection(), m_window);
    }
}

QString XCBEventListener::rotationToString(xcb_randr_rotation_t rotation)
{
    switch (rotation) {
    case XCB_RANDR_ROTATION_ROTATE_0:
        return QStringLiteral("Rotate_0");
    case XCB_RANDR_ROTATION_ROTATE_90:
        return QStringLiteral("Rotate_90");
    case XCB_RANDR_ROTATION_ROTATE_180:
        return QStringLiteral("Rotate_180");
    case XCB_RANDR_ROTATION_ROTATE_270:
        return QStringLiteral("Rotate_270");
    case XCB_RANDR_ROTATION_REFLECT_X:
        return QStringLiteral("Reflect_X");
    case XCB_RANDR_ROTATION_REFLECT_Y:
        return QStringLiteral("Reflect_Y");
    }

    return QStringLiteral("invalid value (%1)").arg(rotation);
}

QString XCBEventListener::connectionToString(xcb_randr_connection_t connection)
{
    switch (connection) {
    case XCB_RANDR_CONNECTION_CONNECTED:
        return QStringLiteral("Connected");
    case XCB_RANDR_CONNECTION_DISCONNECTED:
        return QStringLiteral("Disconnected");
    case XCB_RANDR_CONNECTION_UNKNOWN:
        return QStringLiteral("UnknownConnection");
    }

    return QStringLiteral("invalid value (%1)").arg(connection);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
bool XCBEventListener::nativeEventFilter(const QByteArray &eventType, void *message, long int *result)
#else
bool XCBEventListener::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
#endif
{
    Q_UNUSED(result);

    if (eventType != "xcb_generic_event_t") {
        return false;
    }

    auto *e = static_cast<xcb_generic_event_t *>(message);
    const uint8_t xEventType = e->response_type & ~0x80;

    // If this event is not xcb_randr_notify, we don't want it
    if (xEventType == m_randrBase + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
        handleScreenChange(e);
    }
    if (xEventType == m_randrBase + XCB_RANDR_NOTIFY) {
        handleXRandRNotify(e);
    }

    return false;
}

void XCBEventListener::handleScreenChange(xcb_generic_event_t *e)
{
    auto *e2 = reinterpret_cast<xcb_randr_screen_change_notify_event_t *>(e);

    // Only accept notifications for our window
    if (e2->request_window != m_window) {
        return;
    }

    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "RRScreenChangeNotify";
    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tTimestamp: " << e2->timestamp;
    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tConfig_timestamp: " << e2->config_timestamp;
    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tWindow:" << e2->request_window;
    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tRoot:" << e2->root;
    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tRotation: " << rotationToString((xcb_randr_rotation_t)e2->rotation);
    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tSize ID:" << e2->sizeID;
    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tSize: " << e2->width << e2->height;
    qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tSizeMM: " << e2->mwidth << e2->mheight;

    Q_EMIT screenChanged((xcb_randr_rotation_t)e2->rotation, QSize(e2->width, e2->height), QSize(e2->mwidth, e2->mheight));
    Q_EMIT outputsChanged();
}

void XCBEventListener::handleXRandRNotify(xcb_generic_event_t *e)
{
    auto *randrEvent = reinterpret_cast<xcb_randr_notify_event_t *>(e);

    if (randrEvent->subCode == XCB_RANDR_NOTIFY_CRTC_CHANGE) {
        xcb_randr_crtc_change_t crtc = randrEvent->u.cc;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "RRNotify_CrtcChange";
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tTimestamp: " << crtc.timestamp;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tCRTC: " << crtc.crtc;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tMode: " << crtc.mode;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tRotation: " << rotationToString((xcb_randr_rotation_t)crtc.rotation);
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tGeometry: " << crtc.x << crtc.y << crtc.width << crtc.height;
        Q_EMIT crtcChanged(crtc.crtc, crtc.mode, (xcb_randr_rotation_t)crtc.rotation, QRect(crtc.x, crtc.y, crtc.width, crtc.height), crtc.timestamp);

    } else if (randrEvent->subCode == XCB_RANDR_NOTIFY_OUTPUT_CHANGE) {
        xcb_randr_output_change_t output = randrEvent->u.oc;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "RRNotify_OutputChange";
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tTimestamp: " << output.timestamp;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tOutput: " << output.output;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tCRTC: " << output.crtc;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tMode: " << output.mode;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tRotation: " << rotationToString((xcb_randr_rotation_t)output.rotation);
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tConnection: " << connectionToString((xcb_randr_connection_t)output.connection);
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tSubpixel Order: " << output.subpixel_order;
        Q_EMIT outputChanged(output.output, output.crtc, output.mode, (xcb_randr_connection_t)output.connection);

    } else if (randrEvent->subCode == XCB_RANDR_NOTIFY_OUTPUT_PROPERTY) {
        xcb_randr_output_property_t property = randrEvent->u.op;

        XCB::ScopedPointer<xcb_get_atom_name_reply_t> reply(
            xcb_get_atom_name_reply(QX11Info::connection(), xcb_get_atom_name(QX11Info::connection(), property.atom), nullptr));

        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "RRNotify_OutputProperty (ignored)";
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tTimestamp: " << property.timestamp;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tOutput: " << property.output;
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tProperty: " << xcb_get_atom_name_name(reply.data());
        qCDebug(DEEPIN_DISPLAY_OUTPUT) << "\tState (newValue, Deleted): " << property.status;
    }
}
