#ifndef H_WINDOW_INFO_LOADER_H_
#define H_WINDOW_INFO_LOADER_H_

#include <memory>

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
    std::map<int, std::shared_ptr<WindowInfo>> m_windows;
    QMap<int, QSharedPointer<KWayland::Client::Buffer>> m_windowBuffers;

    void init();
    void updateWindowInfos();
    void onWindowCaptured(int windowId, bool succeed);

public:
    WaylandWindowLoader(KWayland::Client::ClientManagement *clientManagement,
            KWayland::Client::ShmPool *shmPool, QObject *parent = nullptr);
    ~WaylandWindowLoader();

    std::map<int, std::shared_ptr<WindowInfo>> getWindowInfos();

    std::shared_ptr<WindowInfo> getWindowInfo(int windowId);

    bool captureWindow(std::shared_ptr<WindowInfo> info);

Q_SIGNALS:
    void windowCaptured(std::shared_ptr<WindowInfo> info);
};

#endif
