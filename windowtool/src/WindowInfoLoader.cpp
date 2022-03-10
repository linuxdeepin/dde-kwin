#include <clientmanagement.h>
#include <connection_thread.h>
#include <event_queue.h>
#include <registry.h>
#include <shm_pool.h>

#include <QDebug>
#include <QEventLoop>
#include <QImage>
#include <QTimer>

#include "WindowInfoLoader.h"

using namespace KWayland::Client;

WindowInfoLoader *WindowInfoLoader::s_Loader = nullptr;

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

WindowInfoLoader *WindowInfoLoader::loader(QObject *parent)
{
    if (s_Loader == nullptr) {
        s_Loader = new WindowInfoLoader(parent);
    }
    return s_Loader;
}

void WindowInfoLoader::init()
{
    m_xcbLoader = XcbWindowLoader::get();

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
    connect(registry, &Registry::shmAnnounced, this,
            [this, registry](quint32 name, quint32 version) {
                m_shmPool = registry->createShmPool(name, version, this);
            });
    connect(registry, &Registry::clientManagementAnnounced, this,
            [this, registry](quint32 name, quint32 version) {
                m_clientManagement =
                        registry->createClientManagement(name, version, this);
                connect(m_clientManagement, &ClientManagement::captionWindowDone, this,
                        [this](int windowId, bool succeed) {
                            onWindowCaptured(windowId, succeed);
                        });
                connect(m_clientManagement, &ClientManagement::windowStatesChanged, this,
                        [this]() { updateWindowInfos(); });
            });
    connect(registry, &Registry::interfacesAnnounced, this, [this] {
        Q_ASSERT(m_shmPool);
        Q_ASSERT(m_clientManagement);

        m_clientManagement->getWindowStates();
    });

    registry->setEventQueue(m_eventQueue);
    registry->create(m_connection);
    registry->setup();
}

void WindowInfoLoader::updateWindowInfos()
{
    QVector<ClientManagement::WindowState> windowStates =
            m_clientManagement->getWindowStates();

    if (!m_windows.isEmpty()) {
        m_windows.clear();
    }

    for (int i = 0; i < windowStates.count(); ++i) {
        ClientManagement::WindowState state = windowStates.at(i);

        qDebug() << __FUNCTION__ << ":" << __LINE__ << " Add window id " << state.windowId
                 << " pid " << state.pid << " resourceName " << state.resourceName
                 << " isMinimized " << state.isMinimized << " isFullScreen "
                 << state.isFullScreen << " isActive " << state.isActive << " geometry "
                 << state.geometry.width << "x" << state.geometry.height;

        auto info = QSharedPointer<WindowInfo>(new WindowInfo());

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

        m_windows.insert(state.windowId, info);
    }
    if (!m_windows.isEmpty()) {
        inited = true;
    }
}

QSharedPointer<WindowInfo> WindowInfoLoader::getWindowInfo(int windowId)
{
    if (m_windows.contains(windowId)) {
        return m_windows.value(windowId);
    }
    return nullptr;
}

QMap<int, QSharedPointer<WindowInfo>> WindowInfoLoader::getWindowInfos()
{
    while (!inited) {
        QEventLoop eventloop;
        QTimer::singleShot(5, &eventloop, SLOT(quit()));  // wait 5ms
        eventloop.exec();
    }

    return m_windows;
}

bool WindowInfoLoader::captureWindow(QSharedPointer<WindowInfo> info)
{
    if (!m_windows.contains(info->windowId)) {
        return false;
    }
    QSize size(info->width, info->height);
    auto buffer = m_shmPool->getBuffer(size, size.width() * 4).toStrongRef();

    m_windowBuffers.insert(info->windowId, buffer);

    m_clientManagement->getWindowCaption(info->windowId, *buffer);

    return true;
}

void WindowInfoLoader::onWindowCaptured(int windowId, bool succeed)
{
    if (!m_windows.contains(windowId)) {
        qDebug() << __func__ << __LINE__ << " windowId" << windowId << " succeed "
                 << succeed << " but can not find callback";
        return;
    }

    auto buffer = m_windowBuffers.value(windowId);
    auto info   = m_windows.value(windowId);

    if (!succeed) {
        if (!m_xcbLoader) {
            qDebug() << __func__ << __LINE__ << " try get xcb window " << windowId
                     << " resourceName " << info->resourceName << " failed";
            return;
        }

        bool xcb_ok = m_xcbLoader->captureXcbWindow(info);
        if (!xcb_ok) {
            qDebug() << __func__ << __LINE__ << " windowId " << windowId
                     << " resourceName " << info->resourceName << " failed";
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
        fileName += info->resourceName;
        fileName += ".png";

        img.save(fileName, "PNG", 100);
        qDebug() << __func__ << ":" << __LINE__ << " windowId" << windowId << " "
                 << fileName;
    }
}