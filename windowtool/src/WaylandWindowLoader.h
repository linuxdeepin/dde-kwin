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

class WaylandWindowLoader : public QObject
{
    Q_OBJECT
private:
    KWayland::Client::ClientManagement *m_clientManagement = nullptr;
    KWayland::Client::ShmPool *m_shmPool                   = nullptr;

    bool inited = false;
    XcbWindowLoader *m_xcbLoader;
    QMap<int, QSharedPointer<WindowInfo>> m_windows;
    QMap<int, QSharedPointer<KWayland::Client::Buffer>> m_windowBuffers;

    void init();
    void updateWindowInfos();
    void onWindowCaptured(int windowId, bool succeed);

public:
    WaylandWindowLoader(KWayland::Client::ClientManagement *clientManagement,
            QObject *parent = nullptr);
    ~WaylandWindowLoader();

    QMap<int, QSharedPointer<WindowInfo>> getWindowInfos();

    QSharedPointer<WindowInfo> getWindowInfo(int windowId);

    bool captureWindow(QSharedPointer<WindowInfo> info);

    void onProtocolInited();

Q_SIGNALS:
    void windowCaptured(QSharedPointer<WindowInfo> info);
};

#endif
