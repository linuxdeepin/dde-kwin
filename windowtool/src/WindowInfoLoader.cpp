#include <clientmanagement.h>
#include <connection_thread.h>
#include <event_queue.h>
#include <output.h>
#include <registry.h>

#include <QDebug>
#include <QEventLoop>
#include <QThread>

#include "WindowInfoLoader.h"

using namespace KWayland::Client;

WindowInfoLoader *WindowInfoLoader::s_interface = nullptr;

WindowInfoLoader::WindowInfoLoader(QObject *parent)
    : QObject(parent), m_Thread(new QThread(this)), m_connection(new ConnectionThread())
{
    init();
}

WindowInfoLoader::~WindowInfoLoader()
{
    m_Thread->quit();
    m_Thread->wait();
    m_connection->deleteLater();
}

WindowInfoLoader *WindowInfoLoader::get(QObject *parent)
{
    if (s_interface == nullptr) {
        s_interface = new WindowInfoLoader(parent);
    }
    return s_interface;
}

void WindowInfoLoader::init()
{
    XcbWindowLoader::get();  // *init xcb connection first

    connect(
            m_connection, &ConnectionThread::connected, this,
            [this] {
                m_eventQueue = new EventQueue(this);
                m_eventQueue->setup(m_connection);

                Registry *registry = new Registry(this);
                setupRegistry(registry);
            },
            Qt::QueuedConnection);
    m_connection->moveToThread(m_Thread);
    m_Thread->start();

    m_connection->initConnection();
}

void WindowInfoLoader::setupRegistry(Registry *registry)
{
    connect(registry, &Registry::outputAnnounced, this,
            [this, registry](quint32 name, quint32 version) {
                output = registry->createOutput(name, version, this);
                if (output) {
                    qDebug() << __func__ << "Get output" << name;
                    m_outputList << output;
                }
            });

    connect(registry, &Registry::remoteAccessManagerAnnounced, this,
            [this, registry](quint32 name, quint32 version) {
                remoteAccessManager =
                        registry->createRemoteAccessManager(name, version, this);
                m_screenRecorder = QSharedPointer<ScreenRecorder>(
                        new ScreenRecorder(remoteAccessManager, this));

                connect(m_screenRecorder.get(), &ScreenRecorder::bufferCallback, this,
                        &WindowInfoLoader::bufferCallback);
            });

    connect(registry, &Registry::shmAnnounced, this,
            [this, registry](quint32 name, quint32 version) {
                shmPool = registry->createShmPool(name, version, this);
            });

    connect(registry, &Registry::clientManagementAnnounced, this,
            [this, registry](quint32 name, quint32 version) {
                clientManagement = registry->createClientManagement(name, version, this);

                m_waylandWindowLoader = QSharedPointer<WaylandWindowLoader>(
                        new WaylandWindowLoader(clientManagement, this));
                connect(m_waylandWindowLoader.get(), &WaylandWindowLoader::windowCaptured,
                        this, &WindowInfoLoader::bufferCallback);
            });

    connect(registry, &Registry::interfacesAnnounced, this, [this] {
        Q_ASSERT(shmPool);
        Q_ASSERT(clientManagement);
        Q_ASSERT(remoteAccessManager);

        m_waylandWindowLoader->onProtocolInited();
    });

    registry->setEventQueue(m_eventQueue);
    registry->create(m_connection);
    registry->setup();
}

bool WindowInfoLoader::startRecording()
{
    if (!m_screenRecorder) {
        return false;
    }

    return m_screenRecorder->startRecording();
}

bool WindowInfoLoader::screenshot(int count)
{
    if (!m_screenRecorder) {
        return false;
    }

    return m_screenRecorder->screenshot(count);
}

QMap<int, QSharedPointer<WindowInfo>> WindowInfoLoader::getWindowInfos()
{
    QMap<int, QSharedPointer<WindowInfo>> map;
    if (!m_waylandWindowLoader) {
        return map;
    }

    return m_waylandWindowLoader->getWindowInfos();
}

QSharedPointer<WindowInfo> WindowInfoLoader::getWindowInfo(int windowId)
{
    if (!m_waylandWindowLoader) {
        return nullptr;
    }

    return m_waylandWindowLoader->getWindowInfo(windowId);
}

bool WindowInfoLoader::captureWindow(QSharedPointer<WindowInfo> info)
{
    if (!m_waylandWindowLoader) {
        return false;
    }

    return m_waylandWindowLoader->captureWindow(info);
}