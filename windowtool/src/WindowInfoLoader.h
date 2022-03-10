#ifndef H_WINDOW_INFO_LOADER_H_
#define H_WINDOW_INFO_LOADER_H_

#include <QMap>
#include <QObject>
#include <QThread>

#include "WindowInfo.h"
#include "XcbWindowLoader.h"

namespace KWayland {
namespace Client {
class ShmPool;
class ConnectionThread;
class EventQueue;
class ClientManagement;
class Registry;
class Buffer;

}  // namespace Client
}  // namespace KWayland

class WindowInfoLoader : public QObject
{
    Q_OBJECT
private:
    WindowInfoLoader(QObject *parent = nullptr);

    void init();
    void updateWindowInfos();
    void setupRegistry(KWayland::Client::Registry *registry);
    void onWindowCaptured(int windowId, bool succeed);

    QThread *m_Thread                                = nullptr;
    KWayland::Client::ConnectionThread *m_connection = nullptr;
    KWayland::Client::EventQueue *m_eventQueue       = nullptr;

    KWayland::Client::ShmPool *m_shmPool                   = nullptr;
    KWayland::Client::ClientManagement *m_clientManagement = nullptr;

    static WindowInfoLoader *s_Loader;

    bool inited = false;
    XcbWindowLoader *m_xcbLoader;
    QMap<int, QSharedPointer<WindowInfo>> m_windows;
    QMap<int, QSharedPointer<KWayland::Client::Buffer>> m_windowBuffers;

public:
    ~WindowInfoLoader();

    static WindowInfoLoader *loader(QObject *parent = nullptr);

    QMap<int, QSharedPointer<WindowInfo>> getWindowInfos();

    QSharedPointer<WindowInfo> getWindowInfo(int windowId);

    bool captureWindow(QSharedPointer<WindowInfo> info);

Q_SIGNALS:
    void windowCaptured(QSharedPointer<WindowInfo> info);
};

#endif
