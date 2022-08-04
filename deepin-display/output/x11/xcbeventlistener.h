/*
 *  SPDX-FileCopyrightText: 2012, 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef XRANDRX11HELPER_H
#define XRANDRX11HELPER_H

#include <QAbstractNativeEventFilter>
#include <QLoggingCategory>
#include <QObject>
#include <QRect>

#include "xcbwrapper.h"

class XCBEventListener : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    XCBEventListener(xcb_connection_t* pXConnection, xcb_screen_t* pXFirstScreen, xcb_window_t XWindowDummy);
    ~XCBEventListener() override;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray &eventType, void *message, long int *result) override;
#else
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

Q_SIGNALS:
    /* Emitted when only XRandR 1.1 or older is available */
    void screenChanged(xcb_randr_rotation_t rotation, const QSize &sizePx, const QSize &sizeMm);
    void outputsChanged();

    /* Emitted only when XRandR 1.2 or newer is available */
    void crtcChanged(xcb_randr_crtc_t crtc, xcb_randr_mode_t mode, xcb_randr_rotation_t rotation, const QRect &geom, xcb_timestamp_t timestamp);
    void outputChanged(xcb_randr_output_t output, xcb_randr_crtc_t crtc, xcb_randr_mode_t mode, xcb_randr_connection_t connection);
    void outputPropertyChanged(xcb_randr_output_t output);

private:
    QString rotationToString(xcb_randr_rotation_t rotation);
    QString connectionToString(xcb_randr_connection_t connection);
    void handleScreenChange(xcb_generic_event_t *e);
    void handleXRandRNotify(xcb_generic_event_t *e);

protected:
    bool m_isRandrPresent;
    bool m_event11;
    uint8_t m_randrBase;
    uint8_t m_randrErrorBase;
    uint8_t m_majorOpcode;
    uint32_t m_versionMajor;
    uint32_t m_versionMinor;

    uint32_t m_window;
};

Q_DECLARE_LOGGING_CATEGORY(DEEPIN_DISPLAY_OUTPUT)
#endif // XRANDRX11HELPER_H
