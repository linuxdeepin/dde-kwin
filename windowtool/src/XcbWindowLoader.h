// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef H_SRC_XCB_WINDOW_INFO_H_
#define H_SRC_XCB_WINDOW_INFO_H_

#include <memory>

#include <xcb/xcb.h>

#include "WindowInfo.h"

class XcbWindowLoader
{
private:
    XcbWindowLoader();
    XcbWindowLoader(XcbWindowLoader &) = delete;
    XcbWindowLoader &operator=(XcbWindowLoader &) = delete;

    bool init();

    xcb_connection_t *connection;

    static XcbWindowLoader *s_Loader;

public:
    ~XcbWindowLoader();

    static XcbWindowLoader *get();

    Size getWindowSize(std::shared_ptr<WindowInfo> info);
    bool captureXcbWindow(std::shared_ptr<WindowInfo> info);
};

#endif