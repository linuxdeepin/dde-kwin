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
        Maximize = Left|Right|Top|Bottom
    };
    Q_DECLARE_FLAGS(QuickTileMode, QuickTileFlag)

public Q_SLOTS:
    void slotWindowMove();
    void slotWindowMaximize();
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

static QByteArray windowProperty(qulonglong WId, xcb_atom_t propAtom, xcb_atom_t typeAtom)
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

class KWinInterface
{
    typedef int (*ClientMaximizeMode)(const void *);
    typedef void (*ClientMaximize)(void *, KWinUtils::MaximizeMode);
    typedef void (*QuickTileWindow) (void *, KWin::Workspace::QuickTileMode);
public:
    KWinInterface()
    {
        clientMaximizeMode = (ClientMaximizeMode)KWinUtils::resolve("_ZNK4KWin6Client12maximizeModeEv");
        clientMaximize = (ClientMaximize)KWinUtils::resolve("_ZN4KWin14AbstractClient8maximizeENS_12MaximizeModeE");
        quickTileWindow = (QuickTileWindow)KWinUtils::resolve("_ZN4KWin9Workspace15quickTileWindowE6QFlagsINS_13QuickTileFlagEE");
    }

    ClientMaximizeMode clientMaximizeMode;
    ClientMaximize clientMaximize;
    QuickTileWindow quickTileWindow;
};

Q_GLOBAL_STATIC(KWinInterface, interface)

KWinUtils::KWinUtils(QObject *parent)
    : QObject(parent)
{
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

QFunctionPointer KWinUtils::resolve(const char *symbol)
{
    static QString lib_name = "kwin.so." + qApp->applicationVersion();

    return QLibrary::resolve(lib_name, symbol);
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
        interface->quickTileWindow(ws, (KWin::Workspace::QuickTileFlag)side);
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
