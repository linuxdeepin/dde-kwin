#ifndef H_SCREEN_RECORDER_H_
#define H_SCREEN_RECORDER_H_

#include <QObject>

#if HAVE_GBM
#include <gbm.h>
#include <xf86drm.h>
#endif

#include "WindowInfo.h"

namespace KWayland {
namespace Client {
class Output;
class RemoteBuffer;
class RemoteAccessManager;

}  // namespace Client
}  // namespace KWayland

class FrameInfo : public WindowInfo
{
public:
    uint32_t stride;
    int dmaFd = -1;

#if HAVE_GBM
    gbm_bo *bo    = nullptr;
    void *mapData = nullptr;
#endif

    virtual ~FrameInfo();
};

class ScreenRecorder : public QObject
{
    Q_OBJECT
private:
    KWayland::Client::RemoteAccessManager *m_remoteAccessManager = nullptr;

#if HAVE_GBM
    int m_drmFd             = -1;
    gbm_device *m_gbmDevice = nullptr;
#endif

    bool initGbm();
    bool findRenderNode(char *node, size_t maxlen);
    void processBuffer(const KWayland::Client::Output *output,
            const KWayland::Client::RemoteBuffer *remoteBuffer);
    void gbmProcessBuffer(QSharedPointer<FrameInfo> info);

public:
    ScreenRecorder(KWayland::Client::RemoteAccessManager *remoteAccessManager,
            QObject *parent = nullptr);
    ~ScreenRecorder();

    bool startRecording();

    bool screenshot(int count);

Q_SIGNALS:
    void bufferCallback(QSharedPointer<WindowInfo> info);
};

#endif