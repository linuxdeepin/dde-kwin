/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "kwinutils.h"

#include <QLibrary>
#include <QGuiApplication>
#include <QDebug>
#include <QX11Info>
#include <QMargins>
#include <QDateTime>
#include <QQmlEngine>
#include <QQmlContext>
#include <QAbstractNativeEventFilter>

// 为了访问 KWinEffects 的保护成员变量
#define protected public
#include <kwineffects.h>
#undef protected

#include <xcb/xcb.h>
#include <xcb/shape.h>

#include <functional>

static int appVersion()
{
    static int version = -1;

    if (version == -1) {
        // 初始化版本号
        const QStringList &ver_list = qApp->applicationVersion().split(".");
        int app_major = ver_list.value(0).toInt();
        int app_minor = ver_list.value(1).toInt();
        int app_patch = ver_list.value(2).toInt();
        int app_build = ver_list.value(3).toInt();

        version = KWIN_VERSION_CHECK(app_major, app_minor, app_patch, app_build);

        if (version == -1) {
            version =  INT_MAX;
        }
    }

    return version;
}

// KWin全局静态对象
namespace KWin {
class Workspace : public QObject {
public:
    static Workspace *_self;

    enum class QuickTileFlag {
        None = 0,
        Left = 1,
        Right = 1<<1,
        Top = 1<<2,
        Bottom = 1<<3,
        Horizontal = Left|Right,
        Vertical = Top|Bottom,
        Maximize = Left|Right|Top|Bottom,
        TopLeft = Top|Left,
        TopRight = Top|Right,
        BottomLeft = Bottom|Left,
        BottomRight = Bottom|Right
    };
    Q_DECLARE_FLAGS(QuickTileMode, QuickTileFlag)

public Q_SLOTS:
    void slotWindowMove();
    void slotWindowMaximize();
    bool compositing() const;
    void slotTouchPadTomoveWindow(int x, int y);
    void slotEndTouchPadToMoveWindow();

#if !defined(KWIN_VERSION) || KWIN_VERSION < KWIN_VERSION_CHECK(5, 10, 95, 0)
    // kwin < 5.10.95
    void slotWindowQuickTileLeft();
    void slotWindowQuickTileRight();
    void slotWindowQuickTileTop();
    void slotWindowQuickTileBottom();
    void slotWindowQuickTileTopLeft();
    void slotWindowQuickTileTopRight();
    void slotWindowQuickTileBottomLeft();
    void slotWindowQuickTileBottomRight();
#endif
};
class Scripting : public QObject {
public:
    static Scripting *s_self;
};
namespace TabBox {
class TabBox : public QObject {
public:
    static TabBox *s_self;
public Q_SLOTS:
    void slotWalkThroughWindows();
    void slotWalkBackThroughWindows();
};
}

// 开启窗口合成时才会有此对象
extern EffectsHandler* effects;

static Effect *getEffect(const QString &name)
{
    if (!effects)
        return nullptr;

    for (auto i : effects->loaded_effects) {
        if (i.first == name)
            return i.second;
    }

    return nullptr;
}

// 窗口合成器
class Compositor : public QObject
{
public:
    enum SuspendReason { NoReasonSuspend = 0, UserSuspend = 1<<0, BlockRuleSuspend = 1<<1, ScriptSuspend = 1<<2, AllReasonSuspend = 0xff };
    static Compositor *s_compositor;
};

// 光标管理
class Cursor : public QObject
{
public:
    static Cursor *s_self;
};

class AbstractClient : public QObject {};
class Options {
public:
    enum WindowOperation {};
};
class Unmanaged;

namespace Xcb {
class Extensions
{
public:
    static Extensions *s_self;
};
}
}

static inline bool isPlatformX11()
{
    static bool x11 = QX11Info::isPlatformX11();
    return x11;
}

static xcb_atom_t internAtom(const char *name, bool only_if_exists)
{
    if (!name || *name == 0)
        return XCB_NONE;

    if (!isPlatformX11())
        return XCB_NONE;

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(QX11Info::connection(), only_if_exists, strlen(name), name);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(QX11Info::connection(), cookie, 0);

    if (!reply)
        return XCB_NONE;

    xcb_atom_t atom = reply->atom;
    free(reply);

    return atom;
}

static QByteArray atomName(xcb_atom_t atom)
{
    if (!atom)
        return QByteArray();

    if (!isPlatformX11())
        return QByteArray();

    xcb_generic_error_t *error = 0;
    xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(QX11Info::connection(), atom);
    xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(QX11Info::connection(), cookie, &error);
    if (error) {
        // qWarning() << "atomName: bad Atom" << atom;
        free(error);
    }
    if (reply) {
        QByteArray result(xcb_get_atom_name_name(reply), xcb_get_atom_name_name_length(reply));
        free(reply);
        return result;
    }
    return QByteArray();
}

static QByteArray windowProperty(xcb_window_t WId, xcb_atom_t propAtom, xcb_atom_t typeAtom)
{
    if (!isPlatformX11())
        return QByteArray();

    QByteArray data;
    xcb_connection_t* xcb_connection = QX11Info::connection();
    int offset = 0;
    int remaining = 0;

    do {
        xcb_get_property_cookie_t cookie = xcb_get_property(xcb_connection, false, WId,
                                                            propAtom, typeAtom, offset, 1024);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection, cookie, NULL);
        if (!reply)
            break;

        remaining = 0;

        if (reply->type == typeAtom) {
            int len = xcb_get_property_value_length(reply);
            char *datas = (char *)xcb_get_property_value(reply);
            data.append(datas, len);
            remaining = reply->bytes_after;
            offset += len;
        }

        free(reply);
    } while (remaining > 0);

    return data;
}

static void setWindowProperty(xcb_window_t WId, xcb_atom_t propAtom, xcb_atom_t typeAtom, int format, const QByteArray &data)
{
    if (!isPlatformX11())
        return;

    xcb_connection_t* conn = QX11Info::connection();

    if (format == 0 && data.isEmpty()) {
        xcb_delete_property(conn, WId, propAtom);
    } else {
        xcb_change_property(conn, XCB_PROP_MODE_REPLACE, WId, propAtom, typeAtom, format, data.length() * 8 / format, data.constData());
    }
}

static xcb_window_t getParentWindow(xcb_window_t WId)
{
    if (!isPlatformX11())
        return XCB_NONE;

    xcb_connection_t* xcb_connection = QX11Info::connection();
    xcb_query_tree_cookie_t cookie = xcb_query_tree_unchecked(xcb_connection, WId);
    xcb_query_tree_reply_t *reply = xcb_query_tree_reply(xcb_connection, cookie, NULL);

    if (reply) {
        xcb_window_t parent = reply->parent;

        free(reply);

        return parent;
    }

    return XCB_WINDOW_NONE;
}

class KWinInterface
{
    typedef int (*ClientMaximizeMode)(const void *);
    typedef void (*ClientMaximize)(void *, KWinUtils::MaximizeMode);
    typedef void (*ActivateClient)(void*, void*, bool force);
    typedef void (*ClientUpdateCursor)(void *);
    typedef void (*ClientSetDepth)(void*, int);
    typedef void (*ClientCheckNoBorder)(void*);
    typedef void (*QuickTileWindow) (void *, KWin::Workspace::QuickTileMode);
    typedef xcb_cursor_t (*X11CursorGetCursor)(Qt::CursorShape);
    typedef KWin::Options::WindowOperation (*OptionsWindowOperation)(const QString &, bool);
    typedef QObject *(*WorkspaceFindClient)(void *, KWinUtils::Predicate, xcb_window_t);
    typedef QObject *(*WorkspaceFindUnmanaged)(void *, xcb_window_t);
    typedef QObject *(*WorkspaceFindUnmanagedByFunction)(void *, std::function<bool (const KWin::Unmanaged*)>);
    typedef void (*WorkspaceSendPingToWindow)(void *, quint32, quint32);
    typedef int (*XcbExtensionsShapeNotifyEvent)(void*);
    typedef void (*CompositorToggle)(void *, KWin::Compositor::SuspendReason);
    typedef int (*ClientWindowType)(const void*, bool, int);

public:
    KWinInterface()
    {
        clientMaximizeMode = (ClientMaximizeMode)KWinUtils::resolve("_ZNK4KWin6Client12maximizeModeEv");
        clientMaximize = (ClientMaximize)KWinUtils::resolve("_ZN4KWin14AbstractClient8maximizeENS_12MaximizeModeE");
        activateClient = (ActivateClient)KWinUtils::resolve("_ZN4KWin9Workspace14activateClientEPNS_14AbstractClientEb");
        clientUpdateCursor = (ClientUpdateCursor)KWinUtils::resolve("_ZN4KWin14AbstractClient12updateCursorEv");
        clientSetDepth = (ClientSetDepth)KWinUtils::resolve("_ZN4KWin8Toplevel8setDepthEi");
        clientCheckNoBorder = (ClientCheckNoBorder)KWinUtils::resolve("_ZN4KWin6Client13checkNoBorderEv");
        quickTileWindow = (QuickTileWindow)KWinUtils::resolve("_ZN4KWin9Workspace15quickTileWindowE6QFlagsINS_13QuickTileFlagEE");
        x11CursorGetCursor = (X11CursorGetCursor)KWinUtils::resolve("_ZN4KWin6Cursor9x11CursorEN2Qt11CursorShapeE");
        optionsWindowOperation = (OptionsWindowOperation)KWinUtils::resolve("_ZN4KWin7Options15windowOperationERK7QStringb");
        findClient = (WorkspaceFindClient)KWinUtils::resolve("_ZNK4KWin9Workspace10findClientENS_9PredicateEj");
        findUnmanaged = (WorkspaceFindUnmanaged)KWinUtils::resolve("_ZNK4KWin9Workspace13findUnmanagedEj");
        findUnmanagedByFunction = (WorkspaceFindUnmanagedByFunction)KWinUtils::resolve("_ZNK4KWin9Workspace13findUnmanagedESt8functionIFbPKNS_9UnmanagedEEE");
        sendPingToWindow = (WorkspaceSendPingToWindow)KWinUtils::resolve("_ZN4KWin9Workspace16sendPingToWindowEjj");
        xcbExtensionsShapeNotifyEvent = (XcbExtensionsShapeNotifyEvent)KWinUtils::resolve("_ZNK4KWin3Xcb10Extensions16shapeNotifyEventEv");
        compositorSuspend = (CompositorToggle)KWinUtils::resolve("_ZN4KWin10Compositor7suspendENS0_13SuspendReasonE");
        if (!compositorSuspend) {
            compositorSuspend = (CompositorToggle)KWinUtils::resolve("_ZN4KWin13X11Compositor7suspendENS0_13SuspendReasonE");
        }

        compositorResume = (CompositorToggle)KWinUtils::resolve("_ZN4KWin10Compositor6resumeENS0_13SuspendReasonE");
        if (!compositorResume) {
            compositorResume = (CompositorToggle)KWinUtils::resolve("_ZN4KWin13X11Compositor6resumeENS0_13SuspendReasonE");
        }
        clientWindowType = (ClientWindowType)KWinUtils::resolve("_ZNK4KWin6Client10windowTypeEbi");
    }

    ClientWindowType clientWindowType;
    ClientMaximizeMode clientMaximizeMode;
    ClientMaximize clientMaximize;
    ActivateClient activateClient;
    ClientUpdateCursor clientUpdateCursor;
    ClientSetDepth clientSetDepth;
    ClientCheckNoBorder clientCheckNoBorder;
    QuickTileWindow quickTileWindow;
    X11CursorGetCursor x11CursorGetCursor;
    OptionsWindowOperation optionsWindowOperation;
    WorkspaceFindClient findClient;
    WorkspaceFindUnmanaged findUnmanaged;
    WorkspaceFindUnmanagedByFunction findUnmanagedByFunction;
    WorkspaceSendPingToWindow sendPingToWindow;
    XcbExtensionsShapeNotifyEvent xcbExtensionsShapeNotifyEvent;
    CompositorToggle compositorSuspend;
    CompositorToggle compositorResume;
};

Q_GLOBAL_STATIC(KWinInterface, interface)

class KWinUtilsPrivate : public QAbstractNativeEventFilter
{
public:
    KWinUtilsPrivate(KWinUtils *utils)
        : q(utils)
    {
        if (isPlatformX11()) {
            _NET_SUPPORTED = internAtom("_NET_SUPPORTED", false);
        }
    }

    static bool isShapeNotifyEvent(int type)
    {
        if (!KWin::Xcb::Extensions::s_self) {
            return false;
        }

        if (!interface->xcbExtensionsShapeNotifyEvent) {
            return false;
        }

        return interface->xcbExtensionsShapeNotifyEvent(KWin::Xcb::Extensions::s_self) == type;
    }

    void updateWMSupported() {
        if (!isPlatformX11())
            return;

        if (wmSupportedList.isEmpty() && removedWMSupportedList.isEmpty()) {
            return;
        }

        QByteArray net_wm_atoms = windowProperty(QX11Info::appRootWindow(), _NET_SUPPORTED, XCB_ATOM_ATOM);
        QVector<xcb_atom_t> atom_list;

        atom_list.resize(net_wm_atoms.size() / sizeof(xcb_atom_t));
        memcpy(atom_list.data(), net_wm_atoms.constData(), net_wm_atoms.size());

        if (!removedWMSupportedList.isEmpty()) {
            bool removed = false;

            for (xcb_atom_t atom : removedWMSupportedList) {
                if (atom_list.removeAll(atom) > 0) {
                    removed = true;
                }
            }

            if (removed) {
                xcb_change_property(QX11Info::connection(), XCB_PROP_MODE_REPLACE, QX11Info::appRootWindow(),
                                    _NET_SUPPORTED, XCB_ATOM_ATOM, 32, atom_list.size(), atom_list.constData());
            }

            removedWMSupportedList.clear();
        }

        QVector<xcb_atom_t> new_atoms;

        for (xcb_atom_t atom : wmSupportedList) {
            if (!atom_list.contains(atom)) {
                new_atoms.append(atom);;
            }
        }

        if (!new_atoms.isEmpty()) {
            xcb_change_property(QX11Info::connection(), XCB_PROP_MODE_APPEND, QX11Info::appRootWindow(),
                                _NET_SUPPORTED, XCB_ATOM_ATOM, 32, new_atoms.size(), new_atoms.constData());
        }
    }

    void _d_onPropertyChanged(long atom) {
        if (atom != _NET_SUPPORTED)
            return;

        quint64 current_time = QDateTime::currentMSecsSinceEpoch();

        // 防止被短时间内多次调用形成死循环
        if (current_time - lastUpdateTime < 500) {
            lastUpdateTime = current_time;
            return;
        } else {
            lastUpdateTime = current_time;
        }

        updateWMSupported();
    }

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override {
        Q_UNUSED(eventType)
        Q_UNUSED(result)

        xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t*>(message);
        uint response_type = event->response_type & ~0x80;

        if (!isPlatformX11())
            return false;

        if (response_type == XCB_PROPERTY_NOTIFY) {
            xcb_property_notify_event_t *ev = reinterpret_cast<xcb_property_notify_event_t*>(event);

            if (Q_UNLIKELY(monitorProperties.contains(ev->atom))) {
                emit q->windowPropertyChanged(ev->window, ev->atom);
            }

            if (Q_LIKELY(monitorRootWindowProperty)) {
                static long root = QX11Info::appRootWindow();

                if (Q_UNLIKELY(ev->window == root)) {
                    _d_onPropertyChanged(ev->atom);
                }
            }
        } else if (isShapeNotifyEvent(response_type)) {
            xcb_window_t window = reinterpret_cast<xcb_shape_notify_event_t*>(event)->affected_window;

            emit q->windowShapeChanged(window);
        } else if (response_type == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *message = reinterpret_cast<xcb_client_message_event_t*>(event);

            static xcb_atom_t WM_PROTOCOLS = ::internAtom("WM_PROTOCOLS", false);
            static xcb_atom_t _NET_WM_PING = ::internAtom("_NET_WM_PING", false);

            if (message->type == WM_PROTOCOLS && message->data.data32[0] == _NET_WM_PING) {
                xcb_timestamp_t timestamp = message->data.data32[1];
                xcb_window_t window = message->data.data32[2];

                emit q->pingEvent(window, timestamp);
            }
        }

        return false;
    }

    void ensureInstallFilter() {
        if (filterInstalled)
            return;

        filterInstalled = true;
        qApp->installNativeEventFilter(this);
    }

    void maybeRemoveFilter() {
        if (!filterInstalled)
            return;

        if (monitorProperties.isEmpty()) {
            filterInstalled = false;
            qApp->removeNativeEventFilter(this);
        }
    }

    KWinUtils *q;

    QList<xcb_atom_t> wmSupportedList;
    QList<xcb_atom_t> removedWMSupportedList;
    QSet<xcb_atom_t> monitorProperties;
    xcb_atom_t _NET_SUPPORTED;
    qint64 lastUpdateTime = 0;
    bool initialized = false;
    bool filterInstalled = false;
    bool monitorRootWindowProperty = false;
};

KWinUtils::KWinUtils(QObject *parent)
    : QObject(parent)
    , d(new KWinUtilsPrivate(this))
{
#ifdef KWIN_VERSION
    // 往右移动8位是为了排除 build version 字段
    if ((kwinBuildVersion() >> 8) != (kwinRuntimeVersion() >> 8)) {
        // qWarning() << QString("Build on kwin " KWIN_VERSION_STR " version, but run on kwin %1 version").arg(qApp->applicationVersion());
    }
#endif

    if (QObject *ws = workspace()) {
        // 无法从workspace对象获取事件时，则从时间过滤器中取

        if (!connect(ws, SIGNAL(propertyNotify(long)), this, SLOT(_d_onPropertyChanged(long)))) {
            d->monitorRootWindowProperty = true;

            // qDebug() << "KWinUtils: Ignore the 'propertyNotify' signal connect warning";
        }
    }
}

void KWinUtils::setInitialized()
{
    if (d->initialized)
        return;

    d->initialized = true;

    emit initialized();
}

KWinUtils::~KWinUtils()
{

}

KWinUtils *KWinUtils::instance()
{
    static KWinUtils *self = new KWinUtils();

    return self;
}

QObject *KWinUtils::findObjectByClassName(const QByteArray &name, const QObjectList &list)
{
    foreach (QObject *obj, list) {
        if (obj->metaObject()->className() == name) {
            return obj;
        }
    }

    return nullptr;
}

int KWinUtils::kwinBuildVersion()
{
#ifdef KWIN_VERSION
    return KWIN_VERSION;
#endif
    return 0;
}

int KWinUtils::kwinRuntimeVersion()
{
    return appVersion();
}

QObject *KWinUtils::workspace()
{
    return KWin::Workspace::_self;
}

QObject *KWinUtils::compositor()
{
    return KWin::Compositor::s_compositor;
}

QObject *KWinUtils::scripting()
{
    return KWin::Scripting::s_self;
}

void KWinUtils::scriptingRegisterObject(const QString& name, QObject* o)
{
    if (scripting()) {
        auto engine = scripting()->findChild<QQmlEngine*>();
        if (engine) {
            engine->rootContext()->setContextProperty(name, o);
        }
    }
}

QObject *KWinUtils::tabBox()
{
    return KWin::TabBox::TabBox::s_self;
}

QObject *KWinUtils::cursor()
{
    return KWin::Cursor::s_self;
}

QObject *KWinUtils::virtualDesktop()
{
    if (!workspace())
        return nullptr;

    return findObjectByClassName("KWin::VirtualDesktopManager", workspace()->children());
}

namespace KWin {
class Client : public QObject
{

};
}

QObjectList KWinUtils::clientList()
{
    if (!KWinUtils::scripting())
        return {};

    const QObjectList scripting_children = KWinUtils::scripting()->children();
    QObject *jsWorkspaceWrapper = KWinUtils::findObjectByClassName(QByteArrayLiteral("KWin::QtScriptWorkspaceWrapper"), scripting_children);

    if (!jsWorkspaceWrapper) {
        return {};
    }

    QList<KWin::Client*> clients;
    bool ok = QMetaObject::invokeMethod(jsWorkspaceWrapper, "clientList", Q_RETURN_ARG(QList<KWin::Client*>, clients));

    if (!ok) {
        return {};
    }

    QObjectList list;

    for (KWin::Client *c : clients) {
        list << c;
    }

    return list;
}

QObjectList KWinUtils::unmanagedList()
{
    if (!interface->findUnmanagedByFunction || !workspace())
        return {};

    QObjectList list;

    // 在查找函数中将所有Unmanaged对象保存起来
    auto get_all = [&list] (const KWin::Unmanaged *unmanaged) {
        list << reinterpret_cast<QObject*>(const_cast<KWin::Unmanaged*>(unmanaged));
        return false;
    };
    interface->findUnmanagedByFunction(workspace(), get_all);

    return list;
}

QObject *KWinUtils::findClient(KWinUtils::Predicate predicate, quint32 window)
{
    if (!workspace())
        return nullptr;

    if (predicate == Predicate::UnmanagedMatch) {
        if (!interface->findUnmanaged)
            return nullptr;

        return interface->findUnmanaged(workspace(), window);
    }

    if (!interface->findClient)
        return nullptr;

    return interface->findClient(workspace(), predicate, window);
}

void KWinUtils::clientUpdateCursor(QObject *client)
{
    if (interface->clientUpdateCursor) {
        interface->clientUpdateCursor(client);
    }
}

void KWinUtils::setClientDepth(QObject *client, int depth)
{
    if (interface->clientSetDepth) {
        interface->clientSetDepth(client, depth);
    }
}

void KWinUtils::defineWindowCursor(quint32 window, Qt::CursorShape cshape)
{
    if (window == XCB_WINDOW_NONE)
        return;

    if (!interface->x11CursorGetCursor || !isPlatformX11()) {
        return;
    }

    xcb_cursor_t cursor = interface->x11CursorGetCursor(cshape);
    xcb_change_window_attributes(QX11Info::connection(), window, XCB_CW_CURSOR, &cursor);
}

void KWinUtils::clientCheckNoBorder(QObject *client)
{
    if (interface->clientCheckNoBorder) {
        interface->clientCheckNoBorder(client);
    }
}

bool KWinUtils::sendPingToWindow(quint32 WId, quint32 timestamp)
{
    if (!interface->sendPingToWindow)
        return false;

    interface->sendPingToWindow(workspace(), WId, timestamp);
    return true;
}

bool KWinUtils::sendPingToWindow(QObject *client, quint32 timestamp)
{
    bool ok = false;
    xcb_window_t wid = getWindowId(client, &ok);

    if (!ok) {
        return false;
    }

    return sendPingToWindow(wid, timestamp);
}

#if defined(Q_OS_LINUX) && !defined(QT_NO_DYNAMIC_LIBRARY) && !defined(QT_NO_LIBRARY)
QT_BEGIN_NAMESPACE
QFunctionPointer qt_linux_find_symbol_sys(const char *symbol);
QT_END_NAMESPACE
QFunctionPointer KWinUtils::resolve(const char *symbol)
{
    return QT_PREPEND_NAMESPACE(qt_linux_find_symbol_sys)(symbol);
#else
QFunctionPointer KWinUtils::resolve(const char *symbol)
{
    static QString lib_name = "kwin.so." + qApp->applicationVersion();

    return QLibrary::resolve(lib_name, symbol);
#endif
}

qulonglong KWinUtils::getWindowId(const QObject *client, bool *ok)
{
    // kwin class: Toplevel
    return client->property("windowId").toLongLong(ok);
}

int KWinUtils::getWindowDepth(const QObject *client)
{
    bool ok = false;
    xcb_window_t win_id = getWindowId(client, &ok);

    if (!ok)
        return 0;

    if (!isPlatformX11())
        return 0;

    xcb_get_geometry_cookie_t cookit = xcb_get_geometry(QX11Info::connection(), win_id);
    xcb_generic_error_t *error = nullptr;
    xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(QX11Info::connection(), cookit, &error);

    if (error) {
        return 0;
    }

    int depth = reply->depth;
    free(reply);

    return depth;
}

QByteArray KWinUtils::readWindowProperty(quint32 WId, quint32 atom, quint32 type)
{
    return windowProperty(WId, atom, type);
}

QByteArray KWinUtils::readWindowProperty(const QObject *client, quint32 atom, quint32 type)
{
    bool ok = false;
    xcb_window_t wid = getWindowId(client, &ok);

    if (!ok) {
        return QByteArray();
    }

    return windowProperty(wid, atom, type);
}

void KWinUtils::setWindowProperty(quint32 WId, quint32 atom, quint32 type, int format, const QByteArray &data)
{
    ::setWindowProperty(WId, atom, type, format, data);
}

void KWinUtils::setWindowProperty(const QObject *client, quint32 atom, quint32 type, int format, const QByteArray &data)
{
    bool ok = false;
    xcb_window_t wid = getWindowId(client, &ok);

    if (!ok) {
        return;
    }

    ::setWindowProperty(wid, atom, type, format, data);
}

uint KWinUtils::virtualDesktopCount()
{
    if (virtualDesktop()) {
        return virtualDesktop()->property("count").toUInt();
    }

    return 0;
}

uint KWinUtils::currentVirtualDesktop()
{
    if (virtualDesktop()) {
        return virtualDesktop()->property("current").toUInt();
    }

    return 0;
}

bool KWinUtils::compositorIsActive()
{
    QObject *c = compositor();

    if (!c)
        return false;

    QObject *c_dbus = findObjectByClassName("KWin::CompositorDBusInterface", c->children());

    if (!c_dbus) {
        return QX11Info::isCompositingManagerRunning();
    }

    return c_dbus->property("active").toBool();
}

quint32 KWinUtils::internAtom(const QByteArray &name, bool only_if_exists)
{
    return ::internAtom(name.constData(), only_if_exists);
}

quint32 KWinUtils::getXcbAtom(const QString &name, bool only_if_exists) const
{
    return internAtom(name.toLatin1(), only_if_exists);
}

bool KWinUtils::isSupportedAtom(quint32 atom) const
{
    if (atom == XCB_ATOM_NONE) {
        return false;
    }

    static xcb_atom_t _NET_SUPPORTED = internAtom("_NET_SUPPORTED", true);

    if (_NET_SUPPORTED == XCB_ATOM_NONE) {
        return false;
    }

    const QByteArray &datas = windowProperty(QX11Info::appRootWindow(), _NET_SUPPORTED, XCB_ATOM_ATOM);
    const xcb_atom_t *atoms = reinterpret_cast<const xcb_atom_t*>(datas.constData());

    for (int i = 0; i < datas.size() / (int)sizeof(xcb_atom_t); ++i ) {
        if (atoms[i] == atom) {
            return true;
        }
    }

    return false;
}

QVariant KWinUtils::getGtkFrame(const QObject *window) const
{
    if (!window) {
        return QVariant();
    }

    bool ok = false;
    qulonglong wid = getWindowId(window, &ok);

    if (!ok) {
        return QVariant();
    }

    static xcb_atom_t property_atom = internAtom("_GTK_FRAME_EXTENTS", true);

    if (property_atom == XCB_ATOM_NONE) {
        return QVariant();
    }

    const QByteArray &data = windowProperty(wid, property_atom, XCB_ATOM_CARDINAL);

    if (data.size() != 4 * 4)
        return QVariant();

    const int32_t *datas = reinterpret_cast<const int32_t*>(data.constData()); // left right top bottom
    QVariantMap frame_margins {
        {"left", datas[0]},
        {"right", datas[1]},
        {"top", datas[2]},
        {"bottom", datas[3]}
    };

    return frame_margins;
}

bool KWinUtils::isDeepinOverride(const QObject *window) const
{
    bool ok = false;
    qulonglong wid;
    QByteArray data;

    if (!isPlatformX11()) {
        return false;
    }

    static xcb_atom_t property_atom = internAtom("_DEEPIN_OVERRIDE", true);
    if (property_atom == XCB_ATOM_NONE) {
        goto out;
    }

    if (!window) {
        goto out;
    }

    wid = getWindowId(window, &ok);
    if (!ok) {
        goto out;
    }

    data = windowProperty(wid, property_atom, XCB_ATOM_CARDINAL);
    if (data.size() != 4)
        goto out;

    return *reinterpret_cast<const int32_t*>(data.constData()) == 1;

out:
    return false;
}

QVariant KWinUtils::getParentWindow(const QObject *window) const
{
    bool ok = false;
    if (!isPlatformX11()) {
        return QVariant();
    }

    qulonglong wid = getWindowId(window, &ok);

    if (!ok) {
        return QVariant();
    }

    return ::getParentWindow(wid);
}

QVariant KWinUtils::isFullMaximized(const QObject *window) const
{
    if (!interface->clientMaximizeMode) {
        return QVariant();
    }

    return Window::isFullMaximized(window);
}

void KWinUtils::activateClient(QObject *window)
{
    if (interface->activateClient) {
        KWin::Workspace *ws = static_cast<KWin::Workspace *>(workspace());
        interface->activateClient(ws, window, false);
    }
}

QVariant KWinUtils::fullmaximizeWindow(QObject *window) const
{
    if (!interface->clientMaximize) {
        return QVariant();
    }

    return Window::fullmaximizeWindow(window);
}

QVariant KWinUtils::unmaximizeWindow(QObject *window) const
{
    if (!interface->clientMaximize) {
        return QVariant();
    }

    return Window::unmaximizeWindow(window);
}

void KWinUtils::addSupportedProperty(quint32 atom, bool enforce)
{
    if (d->wmSupportedList.contains(atom))
        return;

    d->wmSupportedList.append(atom);

    if (enforce) {
        d->updateWMSupported();
    }
}

void KWinUtils::removeSupportedProperty(quint32 atom, bool enforce)
{
    d->wmSupportedList.removeOne(atom);
    d->removedWMSupportedList.append(atom);

    if (enforce) {
        d->updateWMSupported();
    }
}

void KWinUtils::addWindowPropertyMonitor(quint32 property_atom)
{
    d->monitorProperties.insert(property_atom);
    d->ensureInstallFilter();
}

void KWinUtils::removeWindowPropertyMonitor(quint32 property_atom)
{
    d->monitorProperties.remove(property_atom);
    d->maybeRemoveFilter();
}

bool KWinUtils::isCompositing()
{
    KWin::Workspace *ws = static_cast<KWin::Workspace *>(workspace());
    if (ws) {
        return ws->compositing();
    } else {
        return compositorIsActive();
    }
}

bool KWinUtils::buildNativeSettings(QObject *baseObject, quint32 windowID)
{
    static QFunctionPointer build_function = qApp->platformFunction("_d_buildNativeSettings");

    if (!build_function) {
        return false;
    }

    return reinterpret_cast<bool(*)(QObject*, quint32)>(build_function)(baseObject, windowID);
}

bool KWinUtils::isInitialized() const
{
    return d->initialized;
}

void KWinUtils::WalkThroughWindows()
{
    KWin::TabBox::TabBox *tabbox = static_cast<KWin::TabBox::TabBox *>(tabBox());
    if (tabbox) {
        tabbox->slotWalkThroughWindows();
    }
}

void KWinUtils::WalkBackThroughWindows()
{
    KWin::TabBox::TabBox *tabbox = static_cast<KWin::TabBox::TabBox *>(tabBox());
    if (tabbox) {
        tabbox->slotWalkBackThroughWindows();
    }
}

void KWinUtils::WindowMove()
{
    KWin::Workspace *ws = static_cast<KWin::Workspace *>(workspace());
    if (ws) {
        ws->slotWindowMove();
    }
}

void KWinUtils::TouchPadToMoveWindow(int x, int y)
{
    KWin::Workspace *ws = static_cast<KWin::Workspace *>(workspace());
    if (ws) {
        ws->slotTouchPadTomoveWindow(x,y);
    }
}

void KWinUtils::EndTouchPadToMoveWindow()
{
    KWin::Workspace *ws = static_cast<KWin::Workspace *>(workspace());
    if (ws) {
        ws->slotEndTouchPadToMoveWindow();
    }
}

void KWinUtils::WindowMaximize()
{
    KWin::Workspace *ws = static_cast<KWin::Workspace *>(workspace());
    if (ws) {
        ws->slotWindowMaximize();
    }
}

void KWinUtils::QuickTileWindow(uint side)
{
    KWin::Workspace *ws = static_cast<KWin::Workspace *>(workspace());
    if (ws) {
        if (interface->quickTileWindow) {
            interface->quickTileWindow(ws, (KWin::Workspace::QuickTileFlag)side);
        }
#if !defined(KWIN_VERSION) || KWIN_VERSION < KWIN_VERSION_CHECK(5, 10, 95, 0)
        else { // fallback for kwin < 5.10.95
            switch ((KWin::Workspace::QuickTileFlag)side) {
            case KWin::Workspace::QuickTileFlag::Left:
                ws->slotWindowQuickTileLeft();
                break;
            case KWin::Workspace::QuickTileFlag::Right:
                ws->slotWindowQuickTileRight();;
                break;
            case KWin::Workspace::QuickTileFlag::Top:
                ws->slotWindowQuickTileTop();
                break;
            case KWin::Workspace::QuickTileFlag::Bottom:
                ws->slotWindowQuickTileBottom();
                break;
            case KWin::Workspace::QuickTileFlag::Horizontal:
                ws->slotWindowQuickTileLeft();
                ws->slotWindowQuickTileRight();
                break;
            case KWin::Workspace::QuickTileFlag::Vertical:
                ws->slotWindowQuickTileTop();
                ws->slotWindowQuickTileBottom();
                break;
            case KWin::Workspace::QuickTileFlag::TopLeft:
                ws->slotWindowQuickTileTopLeft();
                break;
            case KWin::Workspace::QuickTileFlag::TopRight:
                ws->slotWindowQuickTileTopRight();
                break;
            case KWin::Workspace::QuickTileFlag::BottomLeft:
                ws->slotWindowQuickTileBottomLeft();
                break;
            case KWin::Workspace::QuickTileFlag::BottomRight:
                ws->slotWindowQuickTileBottomRight();
                break;
            default:
                break;
            }
        }
#endif
    }
}

void KWinUtils::ShowWorkspacesView()
{
    QObject *multitasking = KWin::getEffect("com.deepin.multitasking");

    if (multitasking) {
        QMetaObject::invokeMethod(multitasking, "toggleActive");
    }
}

void KWinUtils::ResumeCompositor(int type)
{
    if (!KWin::Compositor::s_compositor || !interface->compositorResume)
        return;

    interface->compositorResume(KWin::Compositor::s_compositor, static_cast<KWin::Compositor::SuspendReason>(type));
}

void KWinUtils::SuspendCompositor(int type)
{
    if (!KWin::Compositor::s_compositor || !interface->compositorSuspend)
        return;

    interface->compositorSuspend(KWin::Compositor::s_compositor, static_cast<KWin::Compositor::SuspendReason>(type));
}

void KWinUtils::ShowAllWindowsView()
{
    QObject *presentWindows = KWin::getEffect("presentwindows");

    if (presentWindows) {
        QMetaObject::invokeMethod(presentWindows, "toggleActiveAllDesktops");
    }
}

void KWinUtils::ShowWindowsView()
{
    QObject *presentWindows = KWin::getEffect("presentwindows");

    if (presentWindows) {
        QMetaObject::invokeMethod(presentWindows, "toggleActive");
    }
}

bool KWinUtils::Window::isDesktop(const QObject *window)
{
    if (!window) {
        return false;
    }

    if (!interface->clientWindowType) {
        return false;
    }

    return interface->clientWindowType(window, false, 0) == 1;
}

bool KWinUtils::Window::isDock(const QObject *window)
{
    if (!window) {
        return false;
    }

    if (!interface->clientWindowType) {
        return false;
    }

    return interface->clientWindowType(window, false, 0) == 2;
}

bool KWinUtils::Window::isFullMaximized(const QObject *window)
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximizeMode) {
        return false;
    }

    return interface->clientMaximizeMode(window) == MaximizeFull;
}

bool KWinUtils::Window::fullmaximizeWindow(QObject *window)
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximize) {
        return false;
    }

    interface->clientMaximize(window, MaximizeFull);

    return true;
}

bool KWinUtils::Window::unmaximizeWindow(QObject *window)
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximize) {
        return false;
    }

    interface->clientMaximize(window, MaximizeRestore);

    return true;
}

void KWinUtils::Window::setWindowMinimize(QObject *window, bool on)
{
    if (window) {
        window->setProperty("minimized", on);
    }
}

void KWinUtils::Window::closeWindow(QObject *window)
{
    if (window) {
        QMetaObject::invokeMethod(window, "closeWindow");
    }
}

bool KWinUtils::Window::canMaximize(const QObject *window)
{
    return window && window->property("maximizable").toBool();
}

bool KWinUtils::Window::canMinimize(const QObject *window)
{
    return window && window->property("minimizable").toBool();
}

bool KWinUtils::Window::canMove(const QObject *window)
{
    return window && window->property("moveable").toBool();
}

bool KWinUtils::Window::canResize(const QObject *window)
{
    return window && window->property("resizeable").toBool();
}

bool KWinUtils::Window::canClose(const QObject *window)
{
    return window && window->property("closeable").toBool();
}

bool KWinUtils::Window::isKeepAbove(const QObject *window)
{
    return window && window->property("keepAbove").toBool();
}

bool KWinUtils::Window::isSplitscreen(const QObject *window)
{
    return window && window->property("isSplitscreen").toBool();
}

void KWinUtils::Window::setKeepAbove(QObject *window, bool on)
{
    if (!window)
        return;

    window->setProperty("keepAbove", on);
}

bool KWinUtils::Window::isOnAllDesktops(const QObject *window)
{
    return window && window->property("onAllDesktops").toBool();
}

void KWinUtils::Window::setOnAllDesktops(QObject *window, bool on)
{
    if (!window)
        return;

    window->setProperty("onAllDesktops", on);
}

int KWinUtils::Window::windowDesktop(const QObject *window)
{
    if (!window)
        return -1;

    return window->property("desktop").toInt();
}

void KWinUtils::Window::setWindowDesktop(QObject *window, int desktop)
{
    if (window) {
        window->setProperty("desktop", desktop);
    }
}

void KWinUtils::Window::performWindowOperation(QObject *window, const QString &opName, bool restricted)
{
    if (!window || !interface->optionsWindowOperation)
        return;

    KWin::AbstractClient *c = dynamic_cast<KWin::AbstractClient*>(window);
    KWin::Options::WindowOperation op = interface->optionsWindowOperation(opName, restricted);
    QMetaObject::invokeMethod(workspace(), "performWindowOperation", Q_ARG(KWin::AbstractClient*, c), Q_ARG(KWin::Options::WindowOperation, op));
}

void KWinUtils::Window::setQuikTileMode(QObject *window, int m, bool isShowReview)
{
    KWin::AbstractClient *c = dynamic_cast<KWin::AbstractClient*>(window);

    QMetaObject::invokeMethod(workspace(), "slotSetClientSplit", Q_ARG(KWin::AbstractClient*, c), Q_ARG(int, m), Q_ARG(bool, isShowReview));
}

#include "moc_kwinutils.cpp"
