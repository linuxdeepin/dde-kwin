// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <clientmanagement.h>
#include <connection_thread.h>
#include <event_queue.h>
#include <registry.h>
#include <shm_pool.h>

#include <QDebug>
#include <QEventLoop>
#include <QImage>
#include <QTimer>

#include "WaylandWindowLoader.h"
#include "WindowInfoLoader.h"

using namespace KWayland::Client;

WaylandWindowLoader::WaylandWindowLoader(
        ClientManagement *clientManagement, ShmPool *shmPool, QObject *parent)
    : m_clientManagement(clientManagement), m_shmPool(shmPool), QObject(parent)
{
    init();
}

WaylandWindowLoader::~WaylandWindowLoader() {}

void WaylandWindowLoader::init()
{
    m_xcbLoader = XcbWindowLoader::get();

    connect(m_clientManagement, &ClientManagement::captionWindowDone, this,
            [this](int windowId, bool succeed) { onWindowCaptured(windowId, succeed); });
    connect(m_clientManagement, &ClientManagement::windowStatesChanged, this, [this]() {
        if (!inited) {
            updateWindowInfos();
        }
    });
    m_clientManagement->getWindowStates();
}

void WaylandWindowLoader::updateWindowInfos()
{
    QVector<ClientManagement::WindowState> windowStates =
            m_clientManagement->getWindowStates();

    if (!m_windows.empty()) {
        m_windows.clear();
    }

    for (int i = 0; i < windowStates.count(); ++i) {
        ClientManagement::WindowState state = windowStates.at(i);

        qDebug() << __func__ << ":" << __LINE__ << " Add window id " << state.windowId
                 << " pid " << state.pid << " resourceName " << state.resourceName
                 << " isMinimized " << state.isMinimized << " isFullScreen "
                 << state.isFullScreen << " isActive " << state.isActive << " geometry "
                 << state.geometry.width << "x" << state.geometry.height;

        auto info = std::shared_ptr<WindowInfo>(new WindowInfo());

        info->pid          = state.pid;
        info->windowId     = state.windowId;
        info->resourceName = state.resourceName;
        info->x            = state.geometry.x;
        info->y            = state.geometry.y;
        info->width        = state.geometry.width;
        info->height       = state.geometry.height;
        info->isMinimized  = state.isMinimized;
        info->isFullScreen = state.isFullScreen;
        info->isActive     = state.isActive;

        m_windows.insert(
                std::pair<int, std::shared_ptr<WindowInfo>>(state.windowId, info));
    }
    if (!m_windows.empty()) {
        inited = true;
    }
}

std::shared_ptr<WindowInfo> WaylandWindowLoader::getWindowInfo(int windowId)
{
    std::map<int, std::shared_ptr<WindowInfo>>::iterator it;
    it = m_windows.find(windowId);
    if (it != m_windows.end()) {
        m_windows.at(windowId);
    }

    return nullptr;
}

std::map<int, std::shared_ptr<WindowInfo>> WaylandWindowLoader::getWindowInfos()
{
    while (!inited) {
        QEventLoop eventloop;
        QTimer::singleShot(5, &eventloop, SLOT(quit()));  // wait 5ms
        eventloop.exec();
    }

    updateWindowInfos();

    return m_windows;
}

bool WaylandWindowLoader::captureWindow(std::shared_ptr<WindowInfo> info)
{
    if (!m_shmPool) {
        qDebug() << __func__ << __LINE__ << " windowId" << info->windowId
                 << " failed for not shmpool";
        return false;
    }

    std::map<int, std::shared_ptr<WindowInfo>>::iterator it;
    it = m_windows.find(info->windowId);
    if (it == m_windows.end()) {
        return false;
    }

    QSize size(info->width, info->height);
    auto buffer = m_shmPool->getBuffer(size, size.width() * 4).toStrongRef();

    m_windowBuffers.insert(info->windowId, buffer);

    m_clientManagement->getWindowCaption(info->windowId, *buffer);

    return true;
}

void WaylandWindowLoader::onWindowCaptured(int windowId, bool succeed)
{
    qDebug() << __func__ << __LINE__;
    std::map<int, std::shared_ptr<WindowInfo>>::iterator it;
    it = m_windows.find(windowId);
    if (it == m_windows.end()) {
        qDebug() << __func__ << __LINE__ << " windowId" << windowId << " succeed "
                 << succeed << " but can not find callback";
        return;
    }

    auto buffer = m_windowBuffers.value(windowId);
    auto info   = m_windows.at(windowId);

    if (!succeed) {
        if (!m_xcbLoader) {
            qDebug() << __func__ << __LINE__ << " try get xcb window " << windowId
                     << " resourceName " << info->resourceName.c_str() << " failed";
            return;
        }

        bool xcb_ok = m_xcbLoader->captureXcbWindow(info);
        if (!xcb_ok) {
            qDebug() << __func__ << __LINE__ << " windowId " << windowId
                     << " resourceName " << info->resourceName.c_str() << " failed";
            return;
        }
    } else {
        info->winType = WindowInfo::WINDOW_TYPE::WIN_WAYLAND;
        info->data    = buffer->address();
        info->format  = QImage::Format_ARGB32_Premultiplied;
    }

    emit windowCaptured(info);

    bool debug = false;
    if (debug) {
        QImage img((uchar *)info->data, info->width, info->height,
                (QImage::Format)info->format);

        QString fileName = "/tmp/window_screenshot_";
        fileName += QString::number(windowId);
        fileName += "_";
        fileName += info->resourceName.c_str();
        fileName += ".png";

        img.save(fileName, "PNG", 100);
        qDebug() << __func__ << ":" << __LINE__ << " windowId" << windowId << " "
                 << fileName;
    }
}