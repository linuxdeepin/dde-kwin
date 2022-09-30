// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDebug>
#include <QImage>

#include "XcbWindowLoader.h"

XcbWindowLoader *XcbWindowLoader::s_Loader = nullptr;

XcbWindowLoader::XcbWindowLoader()
{
    init();
}

XcbWindowLoader::~XcbWindowLoader() {}

XcbWindowLoader *XcbWindowLoader::get()
{
    if (s_Loader == nullptr) {
        s_Loader = new XcbWindowLoader;
    }
    return s_Loader;
}

bool XcbWindowLoader::init()
{
    if (connection) {
        return true;
    }

    connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection) || !connection) {
        qDebug() << __func__ << ":" << __LINE__ << " xcb_connect failed.";
        return false;
    }
    return true;
}

Size XcbWindowLoader::getWindowSize(std::shared_ptr<WindowInfo> info)
{
    if (!init()) {
        return Size();
    }

    int width, height;
    xcb_generic_error_t *err = nullptr;

    xcb_get_geometry_cookie_t gg_cookie = xcb_get_geometry(connection, info->windowId);
    xcb_get_geometry_reply_t *gg_reply =
            xcb_get_geometry_reply(connection, gg_cookie, &err);
    if (gg_reply) {
        width  = gg_reply->width;
        height = gg_reply->height;
        free(gg_reply);
    } else {
        if (err) {
            qDebug() << __func__ << ":" << __LINE__ << " xcb_get_geometry error "
                     << err->error_code;
            free(err);
        }
        return Size(info->width, info->height);
    }
    return Size(width, height);
}

bool XcbWindowLoader::captureXcbWindow(std::shared_ptr<WindowInfo> info)
{
    if (!init()) {
        return false;
    }

    Size size = getWindowSize(info);
    if (size.isEmpty()) {
        return false;
    }

    xcb_generic_error_t *err = nullptr;
    xcb_get_image_cookie_t gi_cookie =
            xcb_get_image(connection, XCB_IMAGE_FORMAT_Z_PIXMAP, info->windowId, 0, 0,
                    size.width(), size.height(), (uint32_t)(~0UL));
    xcb_get_image_reply_t *gi_reply = xcb_get_image_reply(connection, gi_cookie, &err);
    if (gi_reply) {
        int data_len  = xcb_get_image_data_length(gi_reply);
        uint8_t *data = xcb_get_image_data(gi_reply);

        info->data = data;

        info->format  = QImage::Format_RGB32;
        info->width   = size.width();
        info->height  = size.height();
        info->winType = WindowInfo::WINDOW_TYPE::WIN_X11;
        return true;
    } else {
        if (err) {
            qDebug() << __func__ << ":" << __LINE__ << " xcb_get_image error "
                     << err->error_code;
            free(err);
        }
    }
    return false;
}