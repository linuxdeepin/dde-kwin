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

class KWinUtils : public QObject
{
    Q_OBJECT
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
        InputIdMatch
    };

    explicit KWinUtils(QObject *parent = nullptr);
    ~KWinUtils();

    static QObject *findObjectByClassName(const QByteArray &name, const QObjectList &list);

    static int kwinBuildVersion();
    static int kwinRuntimeVersion();

    static QObject *workspace();
    static QObject *scripting();
    static QObject *tabBox();
    static QObject *cursor();
    static QObject *virtualDesktop();

    static QObjectList clientList();
    static QObject *findClient(Predicate predicate, quint32 window);
    static void clientUpdateCursor(QObject *client);
    static void defineWindowCursor(quint32 window, Qt::CursorShape cshape);

    static QFunctionPointer resolve(const char *symbol);

    static qulonglong getWindowId(const QObject *client, bool *ok = nullptr);

    static uint virtualDesktopCount();
    static uint currentVirtualDesktop();

    struct Window {
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

    Q_INVOKABLE quint32 getXcbAtom(const QString &name, bool only_if_exists) const;
    Q_INVOKABLE bool isSupportedAtom(quint32 atom) const;
    Q_INVOKABLE QVariant getGtkFrame(const QObject *window) const;

    Q_INVOKABLE QVariant getParentWindow(const QObject *window) const;
    Q_INVOKABLE QVariant isFullMaximized(const QObject *window) const;
    Q_INVOKABLE QVariant fullmaximizeWindow(QObject *window) const;
    Q_INVOKABLE QVariant unmaximizeWindow(QObject *window) const;

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
};

#endif // KWINUTILS_H
