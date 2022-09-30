// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Wayland
#include <output.h>
#include <remote_access.h>

// QT
#include <QDebug>
#include <QImage>
#include <QObject>
#include <QThread>

// system
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ScreenRecorder.h"

using namespace KWayland::Client;

FrameInfo::~FrameInfo()
{
    if (winType == WIN_OUTPUT) {
#if HAVE_GBM
        if (bo) {
            gbm_bo_unmap(bo, mapData);
            gbm_bo_destroy(bo);
            bo      = nullptr;
            mapData = nullptr;
            data    = nullptr;
        }
#endif
        if (data) {
            munmap(data, stride * height);
            data = nullptr;
        }
        if (dmaFd >= 0) {
            close(dmaFd);
        }
    }
}

ScreenRecorder::ScreenRecorder(RemoteAccessManager *remoteAccessManager, QObject *parent)
    : QObject(parent), m_remoteAccessManager(remoteAccessManager)
{
    connect(m_remoteAccessManager, &RemoteAccessManager::bufferReady, this,
            [this](const void *o, const RemoteBuffer *remoteBuffer) {
                Output *output =
                        Output::get(reinterpret_cast<wl_output *>(const_cast<void *>(o)));
                connect(remoteBuffer, &RemoteBuffer::parametersObtained, this,
                        [this, output, remoteBuffer] {
                            processBuffer(output, remoteBuffer);
                        });
            });
}

ScreenRecorder::~ScreenRecorder()
{
#if HAVE_GBM
    if (m_gbmDevice) {
        gbm_device_destroy(m_gbmDevice);
        m_gbmDevice = nullptr;
    }
    if (m_drmFd >= 0) {
        close(m_drmFd);
    }
#endif
}

bool ScreenRecorder::findRenderNode(char *node, size_t maxlen)
{
    bool r = false;
#if HAVE_GBM
    drmDevice *devices[64];

    int n = drmGetDevices2(0, devices, sizeof(devices) / sizeof(devices[0]));
    for (int i = 0; i < n; ++i) {
        drmDevice *dev = devices[i];
        if (!(dev->available_nodes & (1 << DRM_NODE_RENDER))) {
            continue;
        }

        strncpy(node, dev->nodes[DRM_NODE_RENDER], maxlen - 1);
        node[maxlen - 1] = '\0';
        r                = true;
        break;
    }

    drmFreeDevices(devices, n);
#endif
    return r;
}

bool ScreenRecorder::initGbm()
{
#if HAVE_GBM
    if (m_gbmDevice) {
        return true;
    }

    char render_node[256];
    if (!findRenderNode(render_node, sizeof(render_node))) {
        qDebug() << __func__ << __LINE__ << "Failed to find a DRM render node";
        return false;
    }

    qDebug() << "Using render node: " << render_node;

    m_drmFd = open(render_node, O_RDWR);
    if (m_drmFd < 0) {
        qDebug() << __func__ << __LINE__ << "Failed to open drm render node";
        return false;
    }

    m_gbmDevice = gbm_create_device(m_drmFd);
    if (m_gbmDevice) {
        return true;
    }
#endif
    qDebug() << __func__ << __LINE__ << "Failed to create gbm device";
    return false;
}

void ScreenRecorder::processBuffer(const Output *output, const RemoteBuffer *remoteBuffer)
{
    if (!remoteBuffer) {
        qDebug() << __func__ << __LINE__ << " remoteBuffer is nullptr";
        return;
    }

    auto info = std::shared_ptr<FrameInfo>(new FrameInfo());

    info->pid          = 0;
    info->windowId     = 0;
    info->resourceName = "output";

    info->x      = 0;
    info->y      = 0;
    info->width  = remoteBuffer->width();
    info->height = remoteBuffer->height();

    info->isMinimized  = false;
    info->isFullScreen = true;
    info->isActive     = true;

    info->winType = WindowInfo::WIN_OUTPUT;
    info->format  = remoteBuffer->format();

    info->stride = remoteBuffer->stride();
    info->dmaFd  = remoteBuffer->fd();

    unsigned char *mapData = static_cast<unsigned char *>(mmap(
            nullptr, info->stride * info->height, PROT_READ, MAP_SHARED, info->dmaFd, 0));

    if (mapData != MAP_FAILED) {
        qDebug() << __func__ << __LINE__ << "success mmap dmafd " << info->dmaFd
                 << " size " << info->width << "x" << info->height;
        info->data = mapData;
    } else {
        gbmProcessBuffer(info);
    }

    qDebug() << __func__ << __LINE__;

    if (info->data) {
        qDebug() << __func__ << __LINE__;
        info->format = QImage::Format_RGB32;
        emit bufferCallback(info);
    }
}

void ScreenRecorder::gbmProcessBuffer(std::shared_ptr<FrameInfo> info)
{
    if (!initGbm()) {
        qDebug() << __func__ << __LINE__ << " gbm_device is null";
        return;
    }

#if HAVE_GBM
    struct gbm_import_fd_data gdata = {
            .fd     = info->dmaFd,
            .width  = info->width,
            .height = info->height,
            .stride = info->stride,
            .format = info->format,
    };
    gbm_bo *bo = gbm_bo_import(m_gbmDevice, GBM_BO_IMPORT_FD, &gdata, GBM_BO_USE_SCANOUT);

    if (!bo) {
        qDebug() << __func__ << __LINE__ << " gbm_bo_import fail";
        return;
    }

    void *mapData = nullptr;
    void *data    = gbm_bo_map(bo, 0, 0, info->width, info->height, GBM_BO_TRANSFER_READ,
               &info->stride, &mapData);

    qDebug() << __func__ << __LINE__ << " dma fd " << info->dmaFd << " width "
             << info->width << " height " << info->height << " stride " << info->stride;

    if (data != MAP_FAILED) {
        qDebug() << __func__ << __LINE__ << " success dma fd " << info->dmaFd << " size "
                 << info->width << "x" << info->height;
        info->data    = data;
        info->mapData = mapData;
        info->bo      = bo;
    }
#endif
}

bool ScreenRecorder::startRecording()
{
    if (!m_remoteAccessManager) {
        return false;
    }

    return m_remoteAccessManager->startRecording(-1);
}

bool ScreenRecorder::screenshot(int count)
{
    if (!m_remoteAccessManager) {
        return false;
    }

    return m_remoteAccessManager->startRecording(count);
}