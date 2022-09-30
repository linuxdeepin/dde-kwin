// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef H_INTERFACE_MANAGER_H_
#define H_INTERFACE_MANAGER_H_

#include <map>
#include <memory>
#include <vector>

#include "WindowInfo.h"

class BufferHandler
{
public:
    virtual void onInited()                                       = 0;
    virtual void bufferCallback(std::shared_ptr<WindowInfo> info) = 0;
};

class WindowInfoLoader
{
private:
    WindowInfoLoader(int argc, char **argv);
    WindowInfoLoader(WindowInfoLoader &) = delete;
    WindowInfoLoader &operator=(WindowInfoLoader &) = delete;

    static WindowInfoLoader *s_interface;

    std::vector<BufferHandler *> m_bufferHandlers;
    void *m_private = nullptr;

public:
    virtual ~WindowInfoLoader();

    static WindowInfoLoader *get(int argc, char **argv);

    void onInited();
    void bufferCallback(std::shared_ptr<WindowInfo> info);

    void addBufferHandler(BufferHandler *handler);
    void removeBufferHandler(BufferHandler *handler);

    bool startRecording();
    bool screenshot(int count);
    int run();

    std::map<int, std::shared_ptr<WindowInfo>> getWindowInfos();
    std::shared_ptr<WindowInfo> getWindowInfo(int windowId);
    bool captureWindow(std::shared_ptr<WindowInfo> info);
};

#endif