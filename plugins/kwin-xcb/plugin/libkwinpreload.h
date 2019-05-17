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
    void grabInput();
    bool hasClient();
    bool isMenuClient(const AbstractClient *c) const;

    void show(const QRect& pos, const QWeakPointer<AbstractClient> &cl);
    void close();
};
#endif // USE_DBUS_MENU
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
