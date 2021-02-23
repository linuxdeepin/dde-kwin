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

#ifndef LIBKWINPRELOAD_H
#define LIBKWINPRELOAD_H

#include <QObject>

#include "kwinutils.h"

namespace KWin {
// 覆盖libkwin.so中的符号
// 使用DBUS菜单时，覆盖了Workspace::showWindowMenu，此函数的原本逻辑中将会调用 UserActionsMenu::show
// 由于kwin中对UserActionsMenu对象有多处调用，因此，不使用DBUS菜单时，应该覆盖 UserActionsMenu 的函数来处理菜单显示
class AbstractClient : public QObject {};
#ifdef USE_DBUS_MENU
class Q_DECL_EXPORT Workspace {
    void showWindowMenu(const QRect& pos, AbstractClient* cl);
};
#else
class Q_DECL_EXPORT UserActionsMenu : public QObject {
    bool isShown() const;
    void handleClick(const QPoint& pos);
    void grabInput();
    bool hasClient();
    bool isMenuClient(const AbstractClient *c) const;

    void show(const QRect& pos, const QWeakPointer<AbstractClient> &cl);
    void close();
};

class RuleBook : public QObject
{
    // kwin经常会触发RuleBool::save函数，
    // 此函数中会修改 ~/.config/kwinrulesrc 配置文件，
    // 且函数会在退出前sync此文件的数据，如果磁盘io被占用，
    // kwin主线程将陷入休眠，导致用户界面长时间无法操作。
    // 故此处禁止修改此配置文件。

    // ###(zccrs): 会导致"kde系统设置"应用中的窗口规则列表为空
    // 带来的影响，不能使用kde系统设置更改窗口规则，
    // 否则会导致 /etc/xdg 中预设的窗口规则失效
    void save();
};

#endif // USE_DBUS_MENU


/*
 * kwin:6abd23ed0220b14a4a05fa280869ba3c953565e3
 * Make it possible to have a separate cursor for the tablet
 * Summary:
 * As is KWin only had 1 Cursor which was a singleton. This made it impossible for
 * us to properly implement the tablet (as in drawing tablets) support and show where
 * we're drawing.
 * This patch makes it possible to have different Cursors in KWin, it makes all the
 * current code still follow the mouse but the tablet can still render a cursor.
 */
#if defined(KWIN_VERSION) && KWIN_VERSION >= KWIN_VERSION_CHECK(5, 18, 5, 0)
class Q_DECL_EXPORT Cursors : public QObject {
    void setCurrentCursor(Cursor* cursor);
};
#endif

enum class BuiltInEffect {
    InValid,
    Blur,
};

namespace BuiltInEffects {
    bool supported(BuiltInEffect effect);
}
}

#if !defined(KWIN_VERSION) || KWIN_VERSION <= KWIN_VERSION_CHECK(5, 8, 6, 0)
namespace Plasma
{
// 覆盖此函数，修复在kwin 5.8.6版本中窗口标题栏显示异常
// bug由plasma-framework库中的Svg类生成缓存机制有问题导致
// 重现步骤：
// 1.设置屏幕缩放比为2.0
// 2.注销后删除 .cache 中 plasma 相关的所有缓存文件
// 3.登录后不要打开任何窗口，直接设置屏幕缩放比为1.75
// 4.再次注销登录进入桌面，打开使用系统标题栏的应用即可观察到此bug
class Q_DECL_EXPORT Theme {
    bool findInRectsCache(const QString &image, const QString &element, QRectF &rect) const;
};
}
#endif

#endif // LIBKWINPRELOAD_H
