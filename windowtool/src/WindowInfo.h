#ifndef H_WINDOW_INFO_H_
#define H_WINDOW_INFO_H_

#include <QString>

class WindowInfo
{
public:
    int32_t pid;
    int32_t windowId;
    QString resourceName;

    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    bool isMinimized;
    bool isFullScreen;
    bool isActive;

    enum WINDOW_TYPE { WIN_OUTPUT = 0, WIN_WAYLAND, WIN_X11 } winType;

    int format;
    void *data;

    WindowInfo() {}

    virtual ~WindowInfo()
    {
        if (winType == WIN_X11 && data) {
            data = nullptr;
        }
    }
};

#endif