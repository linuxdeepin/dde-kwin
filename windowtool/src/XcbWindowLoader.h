#ifndef H_SRC_XCB_WINDOW_INFO_H_
#define H_SRC_XCB_WINDOW_INFO_H_

#include <xcb/xcb.h>

#include <QSize>

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

    QSize getWindowSize(QSharedPointer<WindowInfo> info);
    bool captureXcbWindow(QSharedPointer<WindowInfo> info);
};

#endif