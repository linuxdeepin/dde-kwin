#include <algorithm>

#include <clientmanagement.h>
#include <connection_thread.h>
#include <event_queue.h>
#include <output.h>
#include <registry.h>

#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QList>
#include <QObject>
#include <QThread>

#include "ScreenRecorder.h"
#include "WaylandWindowLoader.h"
#include "WindowInfoLoader.h"

using namespace KWayland::Client;

WindowInfoLoader *WindowInfoLoader::s_interface = nullptr;

class Private : public QObject
{
    Q_OBJECT
private:
    QApplication *m_app = nullptr;

    QThread *m_Thread              = nullptr;
    ConnectionThread *m_connection = nullptr;
    EventQueue *m_eventQueue       = nullptr;

    std::vector<Output *> m_outputList;

    WindowInfoLoader *m_loader;

public:
    RemoteAccessManager *remoteAccessManager = nullptr;
    ClientManagement *clientManagement       = nullptr;
    ShmPool *shmPool                         = nullptr;

    std::shared_ptr<ScreenRecorder> m_screenRecorder           = nullptr;
    std::shared_ptr<WaylandWindowLoader> m_waylandWindowLoader = nullptr;

    Private(int argc, char **argv, WindowInfoLoader *loader, QObject *parent = nullptr)
        : QObject(parent),
          m_loader(loader),
          m_app(new QApplication(argc, argv)),
          m_Thread(new QThread(m_app)),
          m_connection(new ConnectionThread())
    {
        init();
    }

    ~Private()
    {
        m_Thread->quit();
        m_Thread->wait();
        m_connection->deleteLater();
        if (m_app) {
            delete m_app;
            m_app = nullptr;
        }
    }

    void init()
    {
        XcbWindowLoader::get();  // *init xcb connection first

        connect(
                m_connection, &ConnectionThread::connected, this,
                [this] {
                    m_eventQueue = new EventQueue(m_app);
                    m_eventQueue->setup(m_connection);

                    Registry *registry = new Registry(m_app);
                    setupRegistry(registry);
                },
                Qt::QueuedConnection);
        m_connection->moveToThread(m_Thread);
        m_Thread->start();

        m_connection->initConnection();
    }

    int run()
    {
        m_app->setApplicationName("windowloader");
        return m_app->exec();
    }

    void setupRegistry(Registry *registry)
    {
        connect(registry, &Registry::outputAnnounced, m_app,
                [this, registry](quint32 name, quint32 version) {
                    Output *output = registry->createOutput(name, version, this);
                    if (output) {
                        qDebug() << __func__ << "Get output" << name;
                        std::vector<Output *>::iterator iter = std::find(
                                m_outputList.begin(), m_outputList.end(), output);
                        if (iter == m_outputList.end()) {
                            m_outputList.push_back(output);
                        }
                    }
                });

        connect(registry, &Registry::remoteAccessManagerAnnounced, m_app,
                [this, registry](quint32 name, quint32 version) {
                    remoteAccessManager =
                            registry->createRemoteAccessManager(name, version, this);
                });

        connect(registry, &Registry::shmAnnounced, m_app,
                [this, registry](quint32 name, quint32 version) {
                    shmPool = registry->createShmPool(name, version, this);
                });

        connect(registry, &Registry::clientManagementAnnounced, m_app,
                [this, registry](quint32 name, quint32 version) {
                    clientManagement =
                            registry->createClientManagement(name, version, this);
                });

        connect(registry, &Registry::interfacesAnnounced, m_app, [this] {
            Q_ASSERT(shmPool);
            Q_ASSERT(clientManagement);
            Q_ASSERT(remoteAccessManager);

            m_waylandWindowLoader = std::shared_ptr<WaylandWindowLoader>(
                    new WaylandWindowLoader(clientManagement, shmPool, m_app));
            connect(m_waylandWindowLoader.get(), &WaylandWindowLoader::windowCaptured,
                    m_app, [this](std::shared_ptr<WindowInfo> info) {
                        m_loader->bufferCallback(info);
                    });

            m_screenRecorder = std::shared_ptr<ScreenRecorder>(
                    new ScreenRecorder(remoteAccessManager, m_app));

            connect(m_screenRecorder.get(), &ScreenRecorder::bufferCallback, m_app,
                    [this](std::shared_ptr<WindowInfo> info) {
                        m_loader->bufferCallback(info);
                    });
            m_loader->onInited();
        });

        registry->setEventQueue(m_eventQueue);
        registry->create(m_connection);
        registry->setup();
    }
};

WindowInfoLoader::WindowInfoLoader(int argc, char **argv)
{
    m_private = new Private(argc, argv, this, nullptr);
}

WindowInfoLoader::~WindowInfoLoader()
{
    if (m_private) {
        delete m_private;
        m_private = nullptr;
    }
}

WindowInfoLoader *WindowInfoLoader::get(int argc, char **argv)
{
    if (s_interface == nullptr) {
        s_interface = new WindowInfoLoader(argc, argv);
    }
    return s_interface;
}

int WindowInfoLoader::run()
{
    Private *p = static_cast<Private *>(m_private);
    if (!p) {
        return -1;
    }
    return p->run();
}

void WindowInfoLoader::addBufferHandler(BufferHandler *handler)
{
    std::vector<BufferHandler *>::iterator iter =
            std::find(m_bufferHandlers.begin(), m_bufferHandlers.end(), handler);
    if (iter == m_bufferHandlers.end()) {
        m_bufferHandlers.push_back(handler);
    }
}

void WindowInfoLoader::removeBufferHandler(BufferHandler *handler)
{
    std::vector<BufferHandler *>::iterator iter =
            std::find(m_bufferHandlers.begin(), m_bufferHandlers.end(), handler);
    if (iter != m_bufferHandlers.end()) {
        m_bufferHandlers.erase(iter);
    }
}

void WindowInfoLoader::onInited()
{
    std::vector<BufferHandler *>::iterator it;
    for (it = m_bufferHandlers.begin(); it != m_bufferHandlers.end(); it++) {
        (*it)->onInited();
    }
}

void WindowInfoLoader::bufferCallback(std::shared_ptr<WindowInfo> info)
{
    std::vector<BufferHandler *>::iterator it;
    for (it = m_bufferHandlers.begin(); it != m_bufferHandlers.end(); it++) {
        (*it)->bufferCallback(info);
    }
}

bool WindowInfoLoader::startRecording()
{
    Private *p = static_cast<Private *>(m_private);
    if (!p || !p->m_screenRecorder) {
        return false;
    }

    return p->m_screenRecorder->startRecording();
}

bool WindowInfoLoader::screenshot(int count)
{
    Private *p = static_cast<Private *>(m_private);
    if (!p || !p->m_screenRecorder) {
        return false;
    }

    return p->m_screenRecorder->screenshot(count);
}

std::map<int, std::shared_ptr<WindowInfo>> WindowInfoLoader::getWindowInfos()
{
    std::map<int, std::shared_ptr<WindowInfo>> map;
    Private *p = static_cast<Private *>(m_private);
    if (!p || !p->m_waylandWindowLoader) {
        return map;
    }

    return p->m_waylandWindowLoader->getWindowInfos();
}

std::shared_ptr<WindowInfo> WindowInfoLoader::getWindowInfo(int windowId)
{
    Private *p = static_cast<Private *>(m_private);
    if (!p || !p->m_waylandWindowLoader) {
        return nullptr;
    }

    return p->m_waylandWindowLoader->getWindowInfo(windowId);
}

bool WindowInfoLoader::captureWindow(std::shared_ptr<WindowInfo> info)
{
    Private *p = static_cast<Private *>(m_private);
    if (!p || !p->m_waylandWindowLoader) {
        return false;
    }

    return p->m_waylandWindowLoader->captureWindow(info);
}

#include "WindowInfoLoader.moc"