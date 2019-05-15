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
#include <QCoreApplication>
#include <QDebug>
#include <QX11Info>
#include <QMargins>

// 为了访问 KWinEffects 的保护成员变量
#define protected public
#include <kwineffects.h>
#undef protected

#include <xcb/xcb.h>

static int appVersion()
{
    static int version = -1;

    if (version == -1) {
        // 初始化版本号
        const QStringList &ver_list = qApp->applicationVersion().split(".");
        int app_major = ver_list.value(0).toInt();
        int app_minor = ver_list.value(1).toInt();
        int app_patch = ver_list.value(2).toInt();

        version = QT_VERSION_CHECK(app_major, app_minor, app_patch);

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

public Q_SLOTS:
    void suspend(Compositor::SuspendReason reason);
    void resume(Compositor::SuspendReason reason);
};

// 光标管理
class Cursor : public QObject
{
public:
    static Cursor *s_self;
};
}

static xcb_atom_t internAtom(const char *name, bool only_if_exists)
{
    if (!name || *name == 0)
        return XCB_NONE;

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(QX11Info::connection(), only_if_exists, strlen(name), name);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(QX11Info::connection(), cookie, 0);

    if (!reply)
        return XCB_NONE;

    xcb_atom_t atom = reply->atom;
    free(reply);

    return atom;
}

static QByteArray windowProperty(xcb_window_t WId, xcb_atom_t propAtom, xcb_atom_t typeAtom)
{
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

static xcb_window_t getParentWindow(xcb_window_t WId)
{
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
    typedef void (*QuickTileWindow) (void *, KWin::Workspace::QuickTileMode);
    typedef void (*ClientUpdateCursor)(void *);
    typedef xcb_cursor_t (*X11CursorGetCursor)(Qt::CursorShape);
public:
    KWinInterface()
    {
        clientMaximizeMode = (ClientMaximizeMode)KWinUtils::resolve("_ZNK4KWin6Client12maximizeModeEv");
        clientMaximize = (ClientMaximize)KWinUtils::resolve("_ZN4KWin14AbstractClient8maximizeENS_12MaximizeModeE");
        quickTileWindow = (QuickTileWindow)KWinUtils::resolve("_ZN4KWin9Workspace15quickTileWindowE6QFlagsINS_13QuickTileFlagEE");
        clientUpdateCursor = (ClientUpdateCursor)KWinUtils::resolve("_ZN4KWin14AbstractClient12updateCursorEv");
        x11CursorGetCursor = (X11CursorGetCursor)KWinUtils::resolve("_ZN4KWin6Cursor12getX11CursorEN2Qt11CursorShapeE");
    }

    ClientMaximizeMode clientMaximizeMode;
    ClientMaximize clientMaximize;
    QuickTileWindow quickTileWindow;
    ClientUpdateCursor clientUpdateCursor;
    X11CursorGetCursor x11CursorGetCursor;
};

Q_GLOBAL_STATIC(KWinInterface, interface)

KWinUtils::KWinUtils(QObject *parent)
    : QObject(parent)
{
#ifdef KWIN_VERSION
    if (kwinBuildVersion() != kwinRuntimeVersion()) {
        qWarning() << QString("Build on kwin " KWIN_VERSION_STR " version, but run on kwin %1 version").arg(qApp->applicationVersion());
    }
#endif
}

KWinUtils::~KWinUtils()
{

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

QObject *KWinUtils::scripting()
{
    return KWin::Scripting::s_self;
}

QObject *KWinUtils::tabBox()
{
    return KWin::TabBox::TabBox::s_self;
}

QObject *KWinUtils::cursor()
{
    return KWin::Cursor::s_self;
}

namespace KWin {
class Client : public QObject
{

};
}

QObjectList KWinUtils::clientList()
{
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

void KWinUtils::clientUpdateCursor(QObject *client)
{
    if (interface->clientUpdateCursor) {
        interface->clientUpdateCursor(client);
    }
}

void KWinUtils::defineWindowCursor(quint32 window, Qt::CursorShape cshape)
{
    if (window == XCB_WINDOW_NONE)
        return;

    if (!interface->x11CursorGetCursor) {
        return;
    }

    xcb_cursor_t cursor = interface->x11CursorGetCursor(cshape);
    xcb_change_window_attributes(QX11Info::connection(), window, XCB_CW_CURSOR, &cursor);
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

quint32 KWinUtils::getXcbAtom(const QString &name, bool only_if_exists) const
{
    return internAtom(name.toLatin1().constData(), only_if_exists);
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
    qulonglong wid = window->property("windowId").toLongLong(&ok);

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

QVariant KWinUtils::getParentWindow(const QObject *window) const
{
    bool ok = false;
    qulonglong wid = window->property("windowId").toLongLong(&ok);

    if (!ok) {
        return QVariant();
    }

    return ::getParentWindow(wid);
}

QVariant KWinUtils::isFullMaximized(const QObject *window) const
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximizeMode) {
        return QVariant();
    }

    return interface->clientMaximizeMode(window) == MaximizeFull;
}

QVariant KWinUtils::fullmaximizeWindow(QObject *window) const
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximize) {
        return QVariant();
    }

    interface->clientMaximize(window, MaximizeFull);

    return true;
}

QVariant KWinUtils::unmaximizeWindow(QObject *window) const
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximize) {
        return QVariant();
    }

    interface->clientMaximize(window, MaximizeRestore);

    return true;
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
    QObject *desktop_grid = KWin::getEffect("desktopgrid");

    if (desktop_grid) {
        QMetaObject::invokeMethod(desktop_grid, "toggle");
    }
}

void KWinUtils::ResumeCompositor(int type)
{
    if (!KWin::Compositor::s_compositor)
        return;

    KWin::Compositor::s_compositor->resume(static_cast<KWin::Compositor::SuspendReason>(type));
}

void KWinUtils::SuspendCompositor(int type)
{
    if (!KWin::Compositor::s_compositor)
        return;

    KWin::Compositor::s_compositor->suspend(static_cast<KWin::Compositor::SuspendReason>(type));
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
