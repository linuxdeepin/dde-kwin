#ifndef H_INTERFACE_MANAGER_H_
#define H_INTERFACE_MANAGER_H_

#include <QList>
#include <QMap>
#include <QObject>
#include <QSharedPointer>

#include "ScreenRecorder.h"
#include "WaylandWindowLoader.h"

namespace KWayland {
namespace Client {
class ShmPool;
class ConnectionThread;
class EventQueue;
class ClientManagement;
class Registry;
class Buffer;
class RemoteAccessManager;

}  // namespace Client
}  // namespace KWayland

class WindowInfoLoader : public QObject
{
    Q_OBJECT
private:
    WindowInfoLoader(QObject *parent);
    WindowInfoLoader(WindowInfoLoader &) = delete;
    WindowInfoLoader &operator=(WindowInfoLoader &) = delete;

    static WindowInfoLoader *s_interface;

    QThread *m_Thread                                = nullptr;
    KWayland::Client::ConnectionThread *m_connection = nullptr;
    KWayland::Client::EventQueue *m_eventQueue       = nullptr;

    QList<KWayland::Client::Output *> m_outputList;

    QSharedPointer<ScreenRecorder> m_screenRecorder           = nullptr;
    QSharedPointer<WaylandWindowLoader> m_waylandWindowLoader = nullptr;

    void init();
    void setupRegistry(KWayland::Client::Registry *registry);

public:
    virtual ~WindowInfoLoader();

    static WindowInfoLoader *get(QObject *parent = nullptr);

    bool startRecording();
    bool screenshot(int count);

    QMap<int, QSharedPointer<WindowInfo>> getWindowInfos();
    QSharedPointer<WindowInfo> getWindowInfo(int windowId);
    bool captureWindow(QSharedPointer<WindowInfo> info);

    KWayland::Client::RemoteAccessManager *remoteAccessManager = nullptr;
    KWayland::Client::ClientManagement *clientManagement       = nullptr;
    KWayland::Client::ShmPool *shmPool                         = nullptr;
    KWayland::Client::Output *output                           = nullptr;

Q_SIGNALS:
    void bufferCallback(QSharedPointer<WindowInfo> info);
};

#endif