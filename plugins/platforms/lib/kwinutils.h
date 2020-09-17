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
#ifndef KWINUTILS_H
#define KWINUTILS_H

#include <QObject>
#include <QVariant>

#define KWIN_VERSION_CHECK(major, minor, patch, build) ((major<<24)|(minor<<16)|(patch<<8)|build)
#ifdef KWIN_VERSION_STR
#define KWIN_VERSION KWIN_VERSION_CHECK(KWIN_VERSION_MAJ, KWIN_VERSION_MIN, KWIN_VERSION_PAT, KWIN_VERSION_BUI)
#endif

struct wl_resource;
class KWinUtilsPrivate;
class Q_DECL_EXPORT KWinUtils : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool initialized READ isInitialized)

public:
    enum MaximizeMode {
        MaximizeRestore    = 0, ///< The window is not maximized in any direction.
        MaximizeVertical   = 1, ///< The window is maximized vertically.
        MaximizeHorizontal = 2, ///< The window is maximized horizontally.
        /// Equal to @p MaximizeVertical | @p MaximizeHorizontal
        MaximizeFull = MaximizeVertical | MaximizeHorizontal
    };

    enum class Predicate {
        WindowMatch,
        WrapperIdMatch,
        FrameIdMatch,
        InputIdMatch,
        UnmanagedMatch
    };

    ~KWinUtils();

    static KWinUtils *instance();
    static QObject *findObjectByClassName(const QByteArray &name, const QObjectList &list);

    static int kwinBuildVersion();
    static int kwinRuntimeVersion();

    static QObject *workspace();
    static QObject *compositor();
    static QObject *scripting();
    static void scriptingRegisterObject(const QString& name, QObject* o);
    static QObject *tabBox();
    static QObject *cursor();
    static QObject *virtualDesktop();
    static QObject *waylandServer();
    static QObject *waylandDisplay();

    static QObjectList clientList();
    static QObjectList unmanagedList();
    static QObject *findClient(Predicate predicate, quint32 window);
    QObject *findShellClient(struct ::wl_resource *resource) const;
    static void clientUpdateCursor(QObject *client);
    static void setClientDepth(QObject *client, int depth);
    static void defineWindowCursor(quint32 window, Qt::CursorShape cshape);
    static void clientCheckNoBorder(QObject *client);
    static void activateClient(QObject *window);

    static QFunctionPointer resolve(const char *symbol);

    static qulonglong getWindowId(const QObject *client, bool *ok = nullptr);
    static int getWindowDepth(const QObject *client);
    static QByteArray readWindowProperty(quint32 WId, quint32 atom, quint32 type);
    static QByteArray readWindowProperty(const QObject *client, quint32 atom, quint32 type);
    static void setWindowProperty(quint32 WId, quint32 atom, quint32 type, int format, const QByteArray &data);
    static void setWindowProperty(const QObject *client, quint32 atom, quint32 type, int format, const QByteArray &data);

    static uint virtualDesktopCount();
    static uint currentVirtualDesktop();

    static bool compositorIsActive();

    struct Window {
        static bool isDesktop(const QObject *window);
        static bool isDock(const QObject *window);

        static bool isFullMaximized(const QObject *window);
        static bool fullmaximizeWindow(QObject *window);
        static bool unmaximizeWindow(QObject *window);
        static void setWindowMinimize(QObject *window, bool on);
        static void closeWindow(QObject *window);

        static bool canMaximize(const QObject *window);
        static bool canMinimize(const QObject *window);
        static bool canMove(const QObject *window);
        static bool canResize(const QObject *window);
        static bool canClose(const QObject *window);

        static bool isKeepAbove(const QObject *window);
        static void setKeepAbove(QObject *window, bool on);
        static bool isOnAllDesktops(const QObject *window);
        static void setOnAllDesktops(QObject *window, bool on);
        static int windowDesktop(const QObject *window);
        static void setWindowDesktop(QObject *window, int desktop);

        static void performWindowOperation(QObject* window, const QString &opName, bool restricted = false);
    };

    static quint32 internAtom(const QByteArray &name, bool only_if_exists);

    Q_INVOKABLE quint32 getXcbAtom(const QString &name, bool only_if_exists) const;
    Q_INVOKABLE bool isSupportedAtom(quint32 atom) const;
    Q_INVOKABLE QVariant getGtkFrame(const QObject *window) const;
    Q_INVOKABLE bool isDeepinOverride(const QObject *window) const;

    Q_INVOKABLE QVariant getParentWindow(const QObject *window) const;
    Q_INVOKABLE QVariant isFullMaximized(const QObject *window) const;
    Q_INVOKABLE QVariant fullmaximizeWindow(QObject *window) const;
    Q_INVOKABLE QVariant unmaximizeWindow(QObject *window) const;

    // enforce为false时表示只把属性加入到待添加列表，但是不设置_NET_SUPPORTED属性
    Q_INVOKABLE void addSupportedProperty(quint32 atom, bool enforce = true);
    // enforce为false时表示只把属性标记为待删除，但是不设置_NET_SUPPORTED属性
    Q_INVOKABLE void removeSupportedProperty(quint32 atom, bool enforce = true);

    Q_INVOKABLE void addWindowPropertyMonitor(quint32 property_atom);
    Q_INVOKABLE void removeWindowPropertyMonitor(quint32 property_atom);

    Q_INVOKABLE bool isCompositing();

    // Warning: 调用 buildNativeSettings，会导致baseObject的QMetaObject对象被更改
    // 无法使用QMetaObject::cast，不能使用QObject::findChild等接口查找子类，也不能使用qobject_cast转换对象指针类型
    Q_INVOKABLE bool buildNativeSettings(QObject *baseObject, quint32 windowID);

    bool isInitialized() const;
    bool initForWayland() const;

public Q_SLOTS:
    void WalkThroughWindows();
    void WalkBackThroughWindows();
    void WindowMove();
    void WindowMaximize();
    void QuickTileWindow(uint side);
    void ShowWorkspacesView();
    void ShowAllWindowsView();
    void ShowWindowsView();
    void ResumeCompositor(int type);
    void SuspendCompositor(int type);

Q_SIGNALS:
    void initialized();
    void windowPropertyChanged(quint32 windowId, quint32 property_atom);
    void windowShapeChanged(quint32 windowId);

    // for wayland
    void shellClientAdded(QObject *client);
    void shellClientRemoved(QObject *client);

protected:
    explicit KWinUtils(QObject *parent = nullptr);

    KWinUtilsPrivate *d;

private:
    void setInitialized();

    friend class Mischievous;
    Q_PRIVATE_SLOT(d, void _d_onPropertyChanged(long))
    Q_PRIVATE_SLOT(d, void _d_onShellClientAdded(KWin::ShellClient*))
    Q_PRIVATE_SLOT(d, void _d_onShellClientRemoved(KWin::ShellClient*))
};

#endif // KWINUTILS_H
