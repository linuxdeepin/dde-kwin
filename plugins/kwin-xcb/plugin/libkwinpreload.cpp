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

#include "libkwinpreload.h"
#include "kwinutils.h"

#include <QDebug>
#include <QRect>
#include <QDBusInterface>
#include <QDBusReply>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QMenu>
#include <QPointer>
#include <QWindow>
#include <QStyleFactory>
#include <QStyle>

// deepin dbus menu
#define MenuDBusService "com.deepin.menu"
#define MenuDBusPath "/com/deepin/menu"
#define MenuDBusManager "com.deepin.menu.Manager"
#define MenuDBusMenu "com.deepin.menu.Menu"

namespace KWin {
struct MenuItem {
    QString id;
    QString text;
    bool enable;
    bool isCheckable;
    bool checked;
};

static QList<MenuItem> getMenuItemInfos(AbstractClient *cl)
{
    QList<MenuItem> menu_items {
        {"minimize", qApp->translate("WindowMenu", "Minimize"),
         KWinUtils::Window::canMinimize(cl), false, false},
        {"maximizeOrRestore", KWinUtils::Window::isFullMaximized(cl)
                    ? qApp->translate("WindowMenu", "Unmaximize")
                    : qApp->translate("WindowMenu", "Maximize"),
         KWinUtils::Window::canMaximize(cl), false, false},
        {"move", qApp->translate("WindowMenu", "Move"),
         KWinUtils::Window::canMove(cl), false, false},
        {"resize", qApp->translate("WindowMenu", "Resize"),
         KWinUtils::Window::canResize(cl), false, false},
        {"always-on-top", qApp->translate("WindowMenu", "Always on Top"),
         true, true, KWinUtils::Window::isKeepAbove(cl)},
        {"all-workspace", qApp->translate("WindowMenu", "Always on Visible Workspace"),
         true, true, KWinUtils::Window::isOnAllDesktops(cl)},
        {"move-left", qApp->translate("WindowMenu", "Move to Workspace Left"),
         KWinUtils::Window::windowDesktop(cl) > 1, false, false},
        {"move-right", qApp->translate("WindowMenu", "Move to Workspace Right"),
         (uint)KWinUtils::Window::windowDesktop(cl) < KWinUtils::virtualDesktopCount(), false, false},
        {"close", qApp->translate("WindowMenu", "Close"),
         KWinUtils::Window::canClose(cl), false, false}
    };

    return menu_items;
}

class Q_DECL_HIDDEN MenuSlot : public QObject
{
    Q_OBJECT
public:
    MenuSlot(KWin::AbstractClient *cl, QObject *parent = nullptr)
        : QObject(parent)
        , m_client(cl)
    {}

    static void onMenuItemInvoked(const QString &id, bool checked, AbstractClient *cl)
    {
        if (id == "minimize") {
            KWinUtils::Window::setWindowMinimize(cl, true);
        } else if (id == "maximizeOrRestore") {
            if (KWinUtils::Window::isFullMaximized(cl)) {
                KWinUtils::Window::unmaximizeWindow(cl);
            } else {
                KWinUtils::Window::fullmaximizeWindow(cl);
            }
        } else if (id == "move") {
            KWinUtils::Window::performWindowOperation(cl, "Move");
        } else if (id == "resize") {
            KWinUtils::Window::performWindowOperation(cl, "Resize");
        } else if (id == "always-on-top") {
            KWinUtils::Window::setKeepAbove(cl, checked);
        } else if (id == "all-workspace") {
            KWinUtils::Window::setOnAllDesktops(cl, checked);
        } else if (id == "move-left") {
            KWinUtils::Window::setWindowDesktop(cl, KWinUtils::Window::windowDesktop(cl) - 1);
        } else if (id == "move-right") {
            KWinUtils::Window::setWindowDesktop(cl, KWinUtils::Window::windowDesktop(cl) + 1);
        } else if (id == "close") {
            KWinUtils::Window::closeWindow(cl);
        }
    }

public slots:
    void onMenuItemInvoked(const QString &id, bool checked)
    {
        onMenuItemInvoked(id, checked, m_client);
    }

private:
    AbstractClient *m_client;
};

#ifdef USE_DBUS_MENU
void Workspace::showWindowMenu(const QRect &pos, AbstractClient *cl)
{
    QDBusInterface manager_interface(MenuDBusService, MenuDBusPath);
    QDBusReply<QDBusObjectPath> menu_path = manager_interface.call("RegisterMenu");

    if (menu_path.error().type() != QDBusError::NoError) {
        qWarning() << menu_path.error();
        return;
    }

    QJsonObject obj;

    obj["x"] = pos.x();
    obj["y"] = pos.y();
    obj["isDockMenu"] = false;
    obj["isScaled"] = false;

    QJsonObject items_obj;
    QJsonArray array;

    for (const MenuItem &item : getMenuItemInfos(cl)) {
        QJsonObject item_obj;

        item_obj["itemId"] = item.id;
        item_obj["itemText"] = item.text;
        item_obj["isActive"] = item.enable;
        item_obj["isCheckable"] = item.isCheckable;
        item_obj["checked"] = item.checked;
        array.append(item_obj);
    }

    items_obj["items"] = array;
    QJsonDocument json_menu_item(items_obj);
    obj["menuJsonContent"] = QString::fromLocal8Bit(json_menu_item.toJson(QJsonDocument::Compact));

    QDBusInterface menu_interface(MenuDBusService, menu_path.value().path(), MenuDBusMenu);
    QJsonDocument json_menu(obj);
    QEventLoop loop;
    MenuSlot menuSlot(cl);

    QObject::connect(&menu_interface, SIGNAL(MenuUnregistered()), &loop, SLOT(quit()));
    QObject::connect(&menu_interface, SIGNAL(ItemInvoked(QString, bool)), &menuSlot, SLOT(onMenuItemInvoked(QString,bool)));
    menu_interface.asyncCall("ShowMenu", QString::fromLocal8Bit(json_menu.toJson(QJsonDocument::Compact)));
    loop.exec();
}
#else
static QPointer<QMenu> _globalWindowMenu;
static AbstractClient *_menuClient = nullptr;

bool UserActionsMenu::isShown() const
{
    return _globalWindowMenu && _globalWindowMenu->isVisible();
}

void UserActionsMenu::grabInput()
{
    if (!_globalWindowMenu)
        return;

    _globalWindowMenu->windowHandle()->setMouseGrabEnabled(true);
    _globalWindowMenu->windowHandle()->setKeyboardGrabEnabled(true);
}

bool UserActionsMenu::hasClient()
{
    return !_menuClient && isShown();
}

bool UserActionsMenu::isMenuClient(const AbstractClient *c) const
{
    if (!c || !_menuClient)
        return false;

    return c == _menuClient;
}

void UserActionsMenu::show(const QRect &pos, const QWeakPointer<AbstractClient> &cl)
{
    QMenu menu;

    _globalWindowMenu = &menu;
    _menuClient = cl.data();

    for (const MenuItem &item : getMenuItemInfos(cl.data())) {
        QAction *action = menu.addAction(item.text);

        action->setProperty("id", item.id);
        action->setCheckable(item.isCheckable);
        action->setChecked(item.checked);
        action->setEnabled(item.enable);
    }

    if (QStyle *s = QStyleFactory::create("dlight")) {
        s->setParent(&menu);
        menu.setStyle(s);
    }

    connect(&menu, &QMenu::triggered, &menu, [] (const QAction *action) {
        MenuSlot::onMenuItemInvoked(action->property("id").toString(), action->isChecked(), _menuClient);
    });

    menu.exec(pos.topLeft());
    _menuClient = nullptr;
}
void UserActionsMenu::close()
{
    if (_globalWindowMenu) {
        _globalWindowMenu->close();
    }
}
#endif // USE_DBUS_MENU
} // namespace KWin

#if !defined(KWIN_VERSION) || KWIN_VERSION <= KWIN_VERSION_CHECK(5, 8, 6, 0)
namespace Plasma
{
bool Theme::findInRectsCache(const QString &, const QString &, QRectF &) const
{
    return false;
}
}
#endif

#include "libkwinpreload.moc"
