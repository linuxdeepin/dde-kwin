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
#include "chameleonconfig.h"
#include "chameleontheme.h"
#include "chameleonshadow.h"
#include "chameleon.h"
#include "chameleonwindowtheme.h"

#include <kwineffects.h>

#include <KConfig>
#include <KConfigGroup>
#include <KDecoration2/Decoration>
#include <KDecoration2/DecoratedClient>

#include <QPainter>
#include <QDebug>
#include <QX11Info>
#include <QGuiApplication>
#include <QTimer>

#include <xcb/xcb.h>
#include <X11/Xlib.h>

#define DDE_FORCE_DECORATE "__dde__force_decorate"
#define DDE_NEED_UPDATE_NOBORDER "__dde__need_update_noborder"

Q_DECLARE_METATYPE(QPainterPath)

ChameleonConfig::ChameleonConfig(QObject *parent)
    : QObject(parent)
{
#ifndef DISBLE_DDE_KWIN_XCB
    m_atom_deepin_chameleon = KWinUtils::internAtom(_DEEPIN_CHAMELEON, false);
    m_atom_deepin_no_titlebar = KWinUtils::internAtom(_DEEPIN_NO_TITLEBAR, false);
    m_atom_deepin_force_decorate = KWinUtils::internAtom(_DEEPIN_FORCE_DECORATE, false);
    m_atom_deepin_scissor_window = KWinUtils::internAtom(_DEEPIN_SCISSOR_WINDOW, false);
    m_atom_kde_net_wm_shadow = KWinUtils::internAtom(_KDE_NET_WM_SHADOW, false);
    m_atom_net_wm_window_type = KWinUtils::internAtom(_NET_WM_WINDOW_TYPE, false);

    if (KWinUtils::instance()->isInitialized()) {
        init();
    } else {
        connect(KWinUtils::instance(), &KWinUtils::initialized, this, &ChameleonConfig::init);
    }
#endif
}

ChameleonConfig *ChameleonConfig::instance()
{
    static ChameleonConfig *self = new ChameleonConfig();

    return self;
}

quint32 ChameleonConfig::atomDeepinChameleon() const
{
    return m_atom_deepin_chameleon;
}

quint32 ChameleonConfig::atomDeepinNoTitlebar() const
{
    return m_atom_deepin_no_titlebar;
}

quint32 ChameleonConfig::atomDeepinScissorWindow() const
{
    return m_atom_deepin_scissor_window;
}

bool ChameleonConfig::isActivated() const
{
    return m_activated;
}

QString ChameleonConfig::theme() const
{
    return m_theme;
}

bool ChameleonConfig::setTheme(QString theme)
{
    if (m_theme == theme)
        return false;

    if (ChameleonTheme::instance()->setTheme(theme)) {
        m_theme = theme;
        emit themeChanged(m_theme);

        if (isActivated()) {
            // 重新为无边框窗口生成阴影
            clearKWinX11ShadowForWindows();
            clearX11ShadowCache();
            buildKWinX11ShadowForNoBorderWindows();
        }
        bool isDark = false;
        if (theme.contains("dark")) {
            isDark = true;
        }

        KWinUtils::setDarkTheme(isDark);

        return true;
    }

    return false;
}

void ChameleonConfig::onConfigChanged()
{
    KConfig config("kwinrc", KConfig::CascadeConfig);
    KConfigGroup group_decoration(&config, "org.kde.kdecoration2");

    bool active = group_decoration.readEntry("library") == "com.deepin.chameleon";

    setActivated(active);

    KConfigGroup group(&config, TARGET_NAME);
    const QString &theme_info = group.readEntry("theme");

    if (setTheme(theme_info) && active) {
        // 窗口主题无改变且当前主题活跃时应当构建阴影
        buildKWinX11ShadowForNoBorderWindows();
    }
}

#define D_KWIN_DEBUG_APP_START_TIME "D_KWIN_DEBUG_APP_START_TIME"
#if KWIN_VERSION_MAJ <= 5 && KWIN_VERSION_MIN < 25 && KWIN_VERSION_MIN < 4
void ChameleonConfig::onClientAdded(KWin::AbstractClient *client)
#else
void ChameleonConfig::onClientAdded(KWin::Window *client)
#endif
{
    QObject *c = reinterpret_cast<QObject*>(client);

    connect(c, SIGNAL(activeChanged()), this, SLOT(updateClientX11Shadow()));
    connect(c, SIGNAL(hasAlphaChanged()), this, SLOT(updateClientX11Shadow()));
    connect(c, SIGNAL(shapedChanged()), this, SLOT(updateClientX11Shadow()));
    connect(c, SIGNAL(geometryChanged()), this, SLOT(updateWindowSize()));

    enforceWindowProperties(c);
    buildKWinX11Shadow(c);
    if (qEnvironmentVariableIsSet(D_KWIN_DEBUG_APP_START_TIME)) {
        debugWindowStartupTime(c);
    }
}

void ChameleonConfig::onUnmanagedAdded(KWin::Unmanaged *client)
{
    QObject *c = reinterpret_cast<QObject*>(client);

    connect(c, SIGNAL(shapedChanged()), this, SLOT(updateClientX11Shadow()));
    connect(c, SIGNAL(geometryChanged()), this, SLOT(updateWindowSize()));

    enforceWindowProperties(c);
    buildKWinX11Shadow(c);
    debugWindowStartupTime(c);
}

void ChameleonConfig::onShellClientAdded(KWin::ShellClient *client)
{
    QObject *c = reinterpret_cast<QObject*>(client);
    connect(c, SIGNAL(windowRadiusChanged()), this, SLOT(updateWindowRadius()));
    enforceWindowProperties(c);
}

void ChameleonConfig::updateWindowRadius()
{
    QObject *window = QObject::sender();
    if (!window) {
        return;
    }

    KWin::EffectWindow *effect = window->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);
    if (!effect) {
        return;
    }

    const QVariant client_radius = window->property("windowRadius");
    if (!client_radius.isValid()) {
        return;
    }
    QPointF window_radius = client_radius.toPointF();
    if (window_radius.isNull()) {
        return;
    }

    const QVariant &effect_window_radius = effect->data(ChameleonConfig::WindowRadiusRole);
    bool need_update = true;

    if (effect_window_radius.isValid()) {
        auto old_window_radius = effect_window_radius.toPointF();

        if (old_window_radius == window_radius) {
            need_update = false;
        }
    }

    if (need_update) {
        // 清理已缓存的旧的窗口mask材质
        effect->setData(ChameleonConfig::WindowMaskTextureRole, QVariant());
        // 设置新的窗口圆角
        if (window_radius.isNull()) {
            effect->setData(ChameleonConfig::WindowRadiusRole, QVariant());
        } else {
            effect->setData(ChameleonConfig::WindowRadiusRole, QVariant::fromValue(window_radius));
        }
    }
}

static bool canForceSetBorder(const QObject *window)
{
    if (!window->property("managed").toBool())
        return false;

    switch (window->property("windowType").toInt()) {
    case NET::Desktop:
    case NET::Dock:
    case NET::TopMenu:
    case NET::Splash:
    case NET::Notification:
    case NET::OnScreenDisplay:
        return false;
    default:
        break;
    }

    return true;
}

void ChameleonConfig::onCompositingToggled(bool active)
{
#ifndef DISBLE_DDE_KWIN_XCB
    if (active && isActivated()) {
        connect(KWin::effects, &KWin::EffectsHandler::windowDataChanged, this, &ChameleonConfig::onWindowDataChanged, Qt::UniqueConnection);
        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_scissor_window);

        // 需要重设窗口的clip path特效数据
        for (QObject *client : KWinUtils::clientList()) {
            updateClientClipPath(client);

            if (!canForceSetBorder(client)) {
                updateClientWindowRadius(client);
            }
        }

        for (QObject *window : KWinUtils::unmanagedList()) {
            updateClientClipPath(window);
            updateClientWindowRadius(window);
        }
    } else {
        KWinUtils::instance()->removeSupportedProperty(m_atom_deepin_scissor_window);
    }
#endif
}

static QObject *findWindow(xcb_window_t xid)
{
    // 先从普通窗口中查找
    QObject *obj = KWinUtils::instance()->findClient(KWinUtils::Predicate::WindowMatch, xid);

    if (!obj) // 再从unmanaged类型窗口中查找
        obj = KWinUtils::instance()->findClient(KWinUtils::Predicate::UnmanagedMatch, xid);

    return obj;
}

void ChameleonConfig::onWindowPropertyChanged(quint32 windowId, quint32 atom)
{
#ifndef DISBLE_DDE_KWIN_XCB
    if (atom == m_atom_deepin_no_titlebar) {
        emit windowNoTitlebarPropertyChanged(windowId);
    } else if (atom == m_atom_deepin_force_decorate) {
        if (QObject *obj = findWindow(windowId))
            updateClientNoBorder(obj);

        emit windowForceDecoratePropertyChanged(windowId);
    } else if (atom == m_atom_deepin_scissor_window) {
        if (QObject *obj = findWindow(windowId))
            updateClientClipPath(obj);

        emit windowScissorWindowPropertyChanged(windowId);
    } else if (atom == m_atom_net_wm_window_type) {
        QObject *client = KWinUtils::instance()->findClient(KWinUtils::Predicate::WindowMatch, windowId);

        if (!client)
            return;

        //NOTE: if a pending window type change, we ignore the next request
        // (meaning multiple consective window type events)
        if (m_pendingWindows.find(client) != m_pendingWindows.end()) {
            return;
        }

        m_pendingWindows.insert(client, windowId);
        emit windowTypeChanged(client);

        bool force_decorate = client->property(DDE_FORCE_DECORATE).toBool();

        if (!force_decorate)
            return;

        setWindowOverrideType(client, false);
    }
#endif
}

void ChameleonConfig::onWindowDataChanged(KWin::EffectWindow *window, int role)
{
    switch (role) {
    case KWin::WindowBlurBehindRole:
    case WindowRadiusRole:
    case WindowClipPathRole:
        updateWindowBlurArea(window, role);
        break;
    default:
        break;
    }
}

void ChameleonConfig::onWindowShapeChanged(quint32 windowId)
{
    QObject *window = findWindow(windowId);

    if (!window)
        return;

    // 避免短时间内大量触发其阴影的更新
    buildKWinX11ShadowDelay(window);
}

void ChameleonConfig::updateWindowNoBorderProperty(QObject *window)
{
    // NOTE:
    // since this slot gets executed in the event loop, there is a chance that 
    // window has already been destroyed as of now. so we need to do double 
    // check here.
    auto kv = m_pendingWindows.find(window);
    if (kv != m_pendingWindows.end()) {
        QObject *client = KWinUtils::instance()->findClient(KWinUtils::Predicate::WindowMatch, kv.value());

        m_pendingWindows.remove(window);
        if (!client) {
            return;
        }
    }

    if (window->property(DDE_NEED_UPDATE_NOBORDER).toBool()) {
        // 清理掉属性，避免下次重复更新
        window->setProperty(DDE_NEED_UPDATE_NOBORDER, QVariant());

        // 应该更新窗口的noBorder属性
        if (window->property(DDE_FORCE_DECORATE).toBool()) {
            window->setProperty("noBorder", false);
        } else {
            // 重设noBorder属性
            KWinUtils::instance()->clientCheckNoBorder(window);
        }
    }
}

// role 代表什么窗口属性的改变引起的函数调用
void ChameleonConfig::updateWindowBlurArea(KWin::EffectWindow *window, int role)
{
    // 如果属性__dde__ignore_blur_behind_changed有效，且函数是因为WindowBlurBehindRole变化被调用
    // 则表示这次变化是由updateWindowBlurArea本身引起的，应该忽略此次调用
    if (role == KWin::WindowBlurBehindRole && window->property("__dde__ignore_blur_behind_changed").isValid()) {
        // 清理标记的属性
        window->setProperty("__dde__ignore_blur_behind_changed", QVariant());
        return;
    }

    QVariant blur_area = window->data(KWin::WindowBlurBehindRole);

    // 如果窗口设置的模糊区域没有改变，且窗口存在已经缓存的数据时使用已缓存的值
    if (role != KWin::WindowBlurBehindRole) {
        const QVariant &cache_blur_area = window->property("__dde__blur_behind_role");

        if (cache_blur_area.isValid()) {
            blur_area = cache_blur_area;
        }
    }

    // 窗口模糊未启用时不用处理
    if (!blur_area.isValid()) {
        // 清理缓存的属性
        window->setProperty("__dde__blur_behind_role", QVariant());
        return;
    }

    const QVariant &window_clip = window->data(WindowClipPathRole);
    const QVariant &window_radius = window->data(WindowRadiusRole);

    QPainterPath path;
    QPointF radius;

    if (window_clip.isValid()) {
        path = qvariant_cast<QPainterPath>(window_clip);
    }

    if (window_radius.isValid()) {
        radius = window_radius.toPointF();
    }

    // 当窗口未设置圆角且未设置clip path时应该恢复窗口的模糊区域
    if (path.isEmpty() && (qIsNull(radius.x()) || qIsNull(radius.y()))) {
        const QVariant &blur_area = window->property("__dde__blur_behind_role");

        if (blur_area.isValid()) {
            // 先清理缓存的属性
            window->setProperty("__dde__blur_behind_role", QVariant());
            window->setData(KWin::WindowBlurBehindRole, blur_area);
        }

        return;
    }

    // 更新窗口的原始模糊数据, 用于后期的恢复
    if (role == KWin::WindowBlurBehindRole
            || !window->property("__dde__blur_behind_role").isValid()) {
        window->setProperty("__dde__blur_behind_role", blur_area);
    }

    // 优先使用窗口裁剪区域，如裁剪区域无效时使用窗口圆角属性
    if (path.isEmpty()) {
        // 模糊区域的圆角处不会进行多采样，此处应该减小区域，防止和窗口border叠加处出现圆角锯齿
        path.addRoundedRect(QRectF(window->rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius.x() + 0.5, radius.y() + 0.5);
    }

    QPainterPath blur_path;
    QRegion blur_region = qvariant_cast<QRegion>(blur_area);

    if (!blur_region.isEmpty()) {
        blur_path.addRegion(blur_region);

        if ((blur_path - path).isEmpty()) {
            // 模糊区域未超出窗口有效区域时不做任何处理
            return;
        }

        // 将模糊区域限制在窗口的裁剪区域内
        blur_path &= path;
    } else {
        blur_path = path;
    }

    blur_region = QRegion(blur_path.toFillPolygon().toPolygon());
    // 标记应该忽略本次窗口模糊区域的变化，防止循环调用
    window->setProperty("__dde__ignore_blur_behind_changed", true);
    window->setData(KWin::WindowBlurBehindRole, blur_region);
    window->setData(WindowMaskTextureRole, QVariant());
}

// 当窗口设置了radius，且未设置clip path时应该在resize时更新窗口的模糊区域
void ChameleonConfig::updateWindowSize()
{
    QObject *window = QObject::sender();

    if (!window)
        return;

    const QSize &old_size = window->property("__dde__old_size").toSize();
    const QSize &size = window->property("size").toSize();

    if (old_size == size)
        return;

    window->setProperty("__dde_old_size", size);

    KWin::EffectWindow *effect = window->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);

    if (!effect) {
        return;
    }

    if (!effect->data(KWin::WindowBlurBehindRole).isValid())
        return;

    if (effect->data(WindowClipPathRole).isValid())
        return;

    if (!effect->data(WindowRadiusRole).isValid())
        return;

    updateWindowBlurArea(effect, 0);
}

void ChameleonConfig::updateClientX11Shadow()
{
    buildKWinX11Shadow(QObject::sender());
}

void ChameleonConfig::updateClientNoBorder(QObject *client, bool allowReset)
{
#ifndef DISBLE_DDE_KWIN_XCB
    const QByteArray &force_decorate = KWinUtils::instance()->readWindowProperty(client, m_atom_deepin_force_decorate, XCB_ATOM_CARDINAL);
    bool set_border = canForceSetBorder(client);

    if (!force_decorate.isEmpty() && force_decorate.at(0)) {
        // 对于不可设置noBorder属性的窗口，必处于noBorder状态
        if (set_border) {
            if (client->property("noBorder").toBool()) {
                // 窗口包含override类型时不可立即设置noBorder属性，需要在窗口类型改变的事件中再进行设置
                if (setWindowOverrideType(client, false)) {
                    // 标记窗口应该在窗口类型改变的事件中更新noBorder属性
                    client->setProperty(DDE_NEED_UPDATE_NOBORDER, true);
                } else {
                    client->setProperty("noBorder", false);
                }
                client->setProperty(DDE_FORCE_DECORATE, true);
            }
        } else {
            client->setProperty(DDE_FORCE_DECORATE, true);
        }
    } else if (client->property(DDE_FORCE_DECORATE).toBool()) {
        client->setProperty(DDE_FORCE_DECORATE, QVariant());

        if (allowReset) {
            // 需要恢复窗口的override类型
            // 重设窗口类型成功后不可立即设置noBorder属性，需要在窗口类型改变的事件中再进行设置
            if (setWindowOverrideType(client, true)) {
                // 标记窗口应该在窗口类型改变的事件中更新noBorder属性
                client->setProperty(DDE_NEED_UPDATE_NOBORDER, true);
            } else {
                KWinUtils::instance()->clientCheckNoBorder(client);
            }
        }
    }
#endif
}

static ChameleonWindowTheme *buildWindowTheme(QObject *window)
{
    for (QObject *child : window->children()) {
        // 对于ChameleonWindowTheme类型的对象，由于其调用了buildNativeSettings，因此会导致其QMetaObject对象被更改
        // 无法使用QMetaObject::cast，因此不能使用QObject::findChild等接口查找子类，也不能使用qobject_cast转换对象指针类型
        // 此处只能根据类名判断来查找ChameleonWindowTheme对象
        if (strcmp(child->metaObject()->className(), ChameleonWindowTheme::staticMetaObject.className()) == 0) {
            return static_cast<ChameleonWindowTheme*>(child);
        }
    }

    // 构建窗口主题设置对象
    return new ChameleonWindowTheme(window, window);
}

// 针对未被管理的窗口，且设置了强制开启窗口修饰时，更新其窗口圆角状态
// 已被管理的窗口会强制开启窗口修饰，其radius属性将由对应的Chameleon对象设置
void ChameleonConfig::updateClientWindowRadius(QObject *client)
{
    // 对于可以强制设置为有border的窗口不用处理
    if (canForceSetBorder(client)) {
        return;
    }

    if (QX11Info::isPlatformX11() && !client->property(DDE_FORCE_DECORATE).toBool()) {
        return;
    }

    KWin::EffectWindow *effect = client->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);

    if (!effect)
        return;

    QPointF window_radius = ChameleonTheme::instance()->themeConfig()->unmanaged.decoration.windowRadius;
    ChameleonWindowTheme *window_theme = buildWindowTheme(client);

    if (!window_theme->property("__connected_for_window_radius").toBool()) {
        auto update = [client, this] {
            updateClientWindowRadius(client);
        };

        connect(window_theme, &ChameleonWindowTheme::themeChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::windowRadiusChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::windowPixelRatioChanged, this, update);
        // 标记为已初始化信号链接
        window_theme->setProperty("__connected_for_window_radius", true);
    }

    window_radius *= window_theme->windowPixelRatio();

    if (window_theme->propertyIsValid(ChameleonWindowTheme::WindowRadiusProperty)) {
        // 如果窗口自定义设置了圆角大小
        window_radius = window_theme->windowRadius();
    } else if (window_theme->propertyIsValid(ChameleonWindowTheme::ThemeProperty)) {
        // 如果窗口自定义了使用哪个主题
        if (auto config_group = ChameleonTheme::instance()->loadTheme(window_theme->theme())) {
            window_radius = config_group->unmanaged.decoration.windowRadius * window_theme->windowPixelRatio();
        }
    }

    const QVariant client_radius = client->property("windowRadius");
    if (client_radius.isValid()) {
        QPointF radius = client_radius.toPointF();
        if (!radius.isNull()) {
            window_radius = radius;
        }
    }

    const QVariant &effect_window_radius = effect->data(ChameleonConfig::WindowRadiusRole);
    bool need_update = true;

    if (effect_window_radius.isValid()) {
        auto old_window_radius = effect_window_radius.toPointF();

        if (old_window_radius == window_radius) {
            need_update = false;
        }
    }

    if (need_update) {
        // 清理已缓存的旧的窗口mask材质
        effect->setData(ChameleonConfig::WindowMaskTextureRole, QVariant());
        // 设置新的窗口圆角
        if (window_radius.isNull()) {
            effect->setData(ChameleonConfig::WindowRadiusRole, QVariant());
        } else {
            effect->setData(ChameleonConfig::WindowRadiusRole, QVariant::fromValue(window_radius));
        }
    }
}

void ChameleonConfig::updateClientClipPath(QObject *client)
{
    KWin::EffectWindow *effect = client->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);

    if (!effect)
        return;

    QPainterPath path;
    const QByteArray &clip_data = effect->readProperty(m_atom_deepin_scissor_window, m_atom_deepin_scissor_window, 8);

    if (!clip_data.isEmpty()) {
        QDataStream ds(clip_data);
        ds >> path;
    }

    if (path.isEmpty()) {
        effect->setData(WindowClipPathRole, QVariant());
    } else {
        effect->setData(WindowClipPathRole, QVariant::fromValue(path));
    }
}

static thread_local QHash<QObject*, qint64> appStartTimeMap;

// 获取此窗口对应进程的启动时间
static QString readPidEnviron(quint32 pid, const QByteArray &env_key) {
    QFile env_file(QString("/proc/%1/environ").arg(pid));

    if (!env_file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QByteArray &env_data = env_file.readAll();

    int start_pos = env_data.startsWith(env_key) ? 0 : env_data.indexOf("\0" + env_key);

    if (start_pos < 0) {
        return {};
    }

    // 补充等号
    start_pos += 1;

    // 重定位到开头的位置
    start_pos += env_key.size();

    // 查找结束的位置
    int end_pos = env_data.indexOf('\0', start_pos + 1);
    if (end_pos < start_pos) {
        return {};
    }

    return env_data.mid(start_pos, end_pos - start_pos);
}

static quint32 readPPid(quint32 pid) {
    QFile status_file(QString("/proc/%1/status").arg(pid));
    if (!status_file.open(QIODevice::ReadOnly)) {
        return 0;
    }

    QTextStream stream(&status_file);
    QString line;
    while (stream.readLineInto(&line)) {
        if (line.startsWith("PPid")) {
            return line.split(":").last().simplified().toUInt();
        }
    }

    return 0;
}

static quint32 getPidByTopLevel(QObject* toplevel) {
    const QByteArray &pid_data = KWinUtils::readWindowProperty(toplevel, KWinUtils::internAtom("_NET_WM_PID", false), XCB_ATOM_CARDINAL);
    return *reinterpret_cast<const quint32*>(pid_data.data());
}

static qint64 appStartTime(QObject *toplevel)
{
    if (!appStartTimeMap.contains(toplevel)) {
        // 清理数据
        QObject::connect(toplevel, &QObject::destroyed, toplevel, [toplevel] {
            appStartTimeMap.remove(toplevel);
        });

        do {
            // 此处使用windowId属性检测此QObject是否为KWin::Toplevel对象，如果w的父对象不是KWin::Toplevel对象
            // 则无法继续后续的操作，此时应当认为无法获取应用程序启动时间
            if (!toplevel->property("windowId").isValid()) {
                appStartTimeMap[toplevel] = 0;
                return 0;
            }
            // 获取pid属性
            quint32 pid = getPidByTopLevel(toplevel);
            // 如果获取窗口对应进程的pid失败
            if (pid == 0) {
                break;
            }

            QString env_data;

            do {
                // 从进程环境变量中初始化此窗口所对应进程的启动时间
                const QString &data = readPidEnviron(pid, D_KWIN_DEBUG_APP_START_TIME);
                if (!data.isEmpty()) {
                    env_data = data;
                    break;
                } else {
                    pid = readPPid(pid);
                }
            } while(pid > 1);

            if (env_data.isEmpty()) {
                break;
            }

            const qint64 timestamp = env_data.toLongLong();

            // 保存时间戳数据
            appStartTimeMap[toplevel] = timestamp;
            return timestamp;
        } while (false);

        // fallback到root窗口属性获取此进程启动时间
        const QByteArray &time_data = KWinUtils::instance()->readWindowProperty(QX11Info::appRootWindow(),
                                                                                KWinUtils::internAtom(D_KWIN_DEBUG_APP_START_TIME, false),
                                                                                XCB_ATOM_CARDINAL);
        if (!time_data.isEmpty()) {
            quint64 start_time = *reinterpret_cast<const quint64*>(time_data.constData());
            appStartTimeMap[toplevel] = start_time;
            return start_time;
        }

        // fallback到kwin自身记录的启动时间, 也就说为kwin设置了D_KWIN_DEBUG_APP_START_TIME环境变量，即表明
        // 将调试所有窗口的启动时间，而无论这个窗口对应进程是否设置了D_KWIN_DEBUG_APP_START_TIME环境变量。此功
        // 能是为了能debug无法通过外部手段为其设置环境变量的程序
        static qint64 kwin_start_time = qgetenv(D_KWIN_DEBUG_APP_START_TIME).toLongLong();
        appStartTimeMap[toplevel] = kwin_start_time;

        return kwin_start_time;
    }

    return appStartTimeMap[toplevel];
}

void ChameleonConfig::debugWindowStartupTime(QObject *toplevel)
{
    // 只在X11平台开启
    if (!QX11Info::isPlatformX11())
        return;

    // 只有能正常获取到启动的时间戳才认为此窗口开启了调试启动时间的功能
    if (!appStartTime(toplevel))
        return;

    const quint32 pid = getPidByTopLevel(toplevel);
    const QString &damage_count_str = readPidEnviron(pid, "_D_CHECKER_DAMAGE_COUNT");
    toplevel->setProperty("_D_CHECKER_DAMAGE_COUNT", damage_count_str.isEmpty() ? 20 : damage_count_str.toInt());

    // 监听窗口请求重绘的事件
    connect(toplevel, SIGNAL(damaged(KWin::Toplevel*, const QRect&)),
            this, SLOT(onToplevelDamaged(KWin::Toplevel*,QRect)), Qt::UniqueConnection);
}

void ChameleonConfig::onToplevelDamaged(KWin::Toplevel *toplevel, const QRect &damage)
{
    Q_UNUSED(damage)
    QObject *w = reinterpret_cast<QObject*>(toplevel);
    // 使用一个定时器，用于检测窗口在500ms之内是否再次进行了绘制操作
    // 如果一个窗口在500ms内未进行绘制，则足以认为其已经处于稳定状态
    // 此时也可以认为程序已经启动完毕，可以以此计算程序启动所消耗的时间
    // 另外需要特别注意的是，我们需要采用其他手段识别对应进程的UI线程未
    // 阻塞，因为如果UI线程阻塞，则也会出现在 500ms内无任何重绘的情况
    // 但是此时并不能说明程序启动已经完成
    QTimer *checker_timer = qvariant_cast<QTimer*>(w->property("_d_checker_timer"));

    if (!checker_timer) {
        const quint32 pid = getPidByTopLevel(w);

        const QString &check_timer_interval_str = readPidEnviron(pid, "_D_CHECKER_TIMER_INTERVAL");
        const int check_timer_interval = check_timer_interval_str.isEmpty() ? 100 : check_timer_interval_str.toInt();

        const QString &ping_time_str = readPidEnviron(pid, "_D_CHECKER_PING_TIME");
        const qint64 ping_time = ping_time_str.isEmpty() ? 50 : ping_time_str.toLongLong();

        const QString &valid_count_str = readPidEnviron(pid, "_D_CHECKER_VALID_COUNT");
        const int valid_count = valid_count_str.isEmpty() ? 10 : valid_count_str.toInt();

        const quint32 timer_used_time = check_timer_interval * valid_count;

        // 不要把w指定为计时器的父对象，它俩很可能属于不同的线程
        checker_timer = new QTimer();
        connect(w, &QObject::destroyed, checker_timer, &QTimer::deleteLater);
        w->setProperty("_d_checker_timer", QVariant::fromValue(checker_timer));
        checker_timer->setInterval(check_timer_interval);
        // 每隔50ms给窗口发送一次ping时间，如若能在100ms内给出回应，则认为本次检测的结果有效
        // 否则将从头进行检测行为
        connect(checker_timer, &QTimer::timeout, w, [w, checker_timer] {
            // 此时说明上一次的检测还未获取到结果，将停止检测等待client返回消息
            if (checker_timer->property("_d_timestamp").isValid()) {
                checker_timer->stop();
                return;
            }

            // 记录发送ping事件的时间
            checker_timer->setProperty("_d_timestamp", QDateTime::currentMSecsSinceEpoch());

            // 使用ping检测client进程是否卡死
            KWinUtils::sendPingToWindow(w, 0);
        });

        connect(KWinUtils::instance(), &KWinUtils::pingEvent, checker_timer,
                [checker_timer, ping_time, valid_count, timer_used_time, w, this] (quint32 windowId, quint32 timestamp) {
            if (timestamp || KWinUtils::getWindowId(w) != windowId)
                return;

            qint64 _d_timestamp = checker_timer->property("_d_timestamp").toLongLong();
            // 清理时间戳属性
            checker_timer->setProperty("_d_timestamp", QVariant());

            if (!_d_timestamp)
                return;

            if (QDateTime::currentMSecsSinceEpoch() - _d_timestamp > ping_time) {
                // 本次ping回复超时，将重启检测
                checker_timer->setProperty("_d_valid_count", 0);
                checker_timer->start();
                return;
            }

            // 记录检测的次数
            int _d_valid_count = checker_timer->property("_d_valid_count").toInt() + 1;
            checker_timer->setProperty("_d_valid_count", _d_valid_count);

            // 表明启动已完成
            if (_d_valid_count >= valid_count) {
                // 销毁定时器对象
                checker_timer->stop();
                checker_timer->deleteLater();
                // 断开无用的链接
                disconnect(w, SIGNAL(damaged(KWin::Toplevel*, const QRect&)),
                           this, SLOT(onToplevelDamaged(KWin::Toplevel*,QRect)));
                qint64 start = appStartTime(w);
                // 结束应用启动时间的调试
                appStartTimeMap[w] = 0;

                qint64 end = QDateTime::currentMSecsSinceEpoch();
                quint32 time = end - start - timer_used_time;
                // 在窗口属性上保存其启动时间的信息
                KWinUtils::setWindowProperty(w, KWinUtils::internAtom("_D_APP_STARTUP_TIME", false),
                                             XCB_ATOM_CARDINAL, 32, QByteArray(reinterpret_cast<char*>(&time), sizeof(time) / sizeof(char)));
            }
        });
    }

    int damage_count = checker_timer->property("_d_damage_count").toInt();
    int _d_damage_count = w->property("_D_CHECKER_DAMAGE_COUNT").toInt();

    // 仅限在前20次绘制中重启检测定时器，client可能会一直处于绘制而未进入稳定状态，比如游戏或视频播放器
    if (++damage_count < _d_damage_count) {
        checker_timer->setProperty("_d_damage_count", damage_count);
        // _d_valid_count 属性用于记录有效的检测次数，累计到10次时完成检测
        // 遇到重回事件时应当将其重置为0
        checker_timer->setProperty("_d_valid_count", 0);
        checker_timer->setProperty("_d_timestamp", QVariant());
        checker_timer->start();
    }
}

void ChameleonConfig::init()
{
#ifndef DISBLE_DDE_KWIN_XCB
    connect(KWinUtils::workspace(), SIGNAL(configChanged()), this, SLOT(onConfigChanged()));
#if KWIN_VERSION_MAJ <= 5 && KWIN_VERSION_MIN < 25 && KWIN_VERSION_MIN < 4
    connect(KWinUtils::workspace(), SIGNAL(windowAdded(KWin::AbstractClient*)), this, SLOT(onClientAdded(KWin::AbstractClient*)));
#else
    connect(KWinUtils::workspace(), SIGNAL(windowAdded(KWin::Window*)), this, SLOT(onClientAdded(KWin::Window*)));
#endif
    connect(KWinUtils::workspace(), SIGNAL(unmanagedAdded(KWin::Unmanaged*)), this, SLOT(onUnmanagedAdded(KWin::Unmanaged*)));
    connect(KWinUtils::compositor(), SIGNAL(compositingToggled(bool)), this, SLOT(onCompositingToggled(bool)));
    connect(KWinUtils::instance(), &KWinUtils::windowPropertyChanged, this, &ChameleonConfig::onWindowPropertyChanged);
    connect(KWinUtils::instance(), &KWinUtils::windowShapeChanged, this, &ChameleonConfig::onWindowShapeChanged);
#endif

    connect(KWinUtils::workspace(), SIGNAL(shellClientAdded(KWin::ShellClient*)), this, SLOT(onShellClientAdded(KWin::ShellClient*)));

    // 初始化链接客户端的信号
    for (QObject *c : KWinUtils::instance()->clientList()) {
        connect(c, SIGNAL(activeChanged()), this, SLOT(updateClientX11Shadow()));
        connect(c, SIGNAL(hasAlphaChanged()), this, SLOT(updateClientX11Shadow()));
        connect(c, SIGNAL(shapedChanged()), this, SLOT(updateClientX11Shadow()));
    }

    for (QObject *c : KWinUtils::instance()->unmanagedList()) {
        connect(c, SIGNAL(shapedChanged()), this, SLOT(updateClientX11Shadow()));
    }

    // 不要立即触发槽，窗口类型改变时，kwin中还未处理此事件，因此需要在下个事件循环中更新窗口的noBorder属性
    connect(this, &ChameleonConfig::windowTypeChanged, this, &ChameleonConfig::updateWindowNoBorderProperty, Qt::QueuedConnection);

    onConfigChanged();
}

void ChameleonConfig::setActivated(const bool active)
{
    if (m_activated == active)
        return;

    m_activated = active;

#ifndef DISBLE_DDE_KWIN_XCB
    if (active) {
        if (KWinUtils::compositorIsActive()) {
            connect(KWin::effects, &KWin::EffectsHandler::windowDataChanged, this, &ChameleonConfig::onWindowDataChanged, Qt::UniqueConnection);

            KWinUtils::instance()->addSupportedProperty(m_atom_deepin_scissor_window, false);
        }

        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_chameleon, false);
        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_no_titlebar, false);
        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_force_decorate);

        // 监听属性变化
        KWinUtils::instance()->addWindowPropertyMonitor(m_atom_deepin_no_titlebar);
        KWinUtils::instance()->addWindowPropertyMonitor(m_atom_deepin_force_decorate);
        KWinUtils::instance()->addWindowPropertyMonitor(m_atom_deepin_scissor_window);
        KWinUtils::instance()->addWindowPropertyMonitor(m_atom_net_wm_window_type);
    } else {
        if (KWin::effects) {
            disconnect(KWin::effects, &KWin::EffectsHandler::windowDataChanged, this, &ChameleonConfig::onWindowDataChanged);
        }

        KWinUtils::instance()->removeSupportedProperty(m_atom_deepin_scissor_window, false);
        KWinUtils::instance()->removeSupportedProperty(m_atom_deepin_chameleon, false);
        KWinUtils::instance()->removeSupportedProperty(m_atom_deepin_no_titlebar, false);
        KWinUtils::instance()->removeSupportedProperty(m_atom_deepin_force_decorate);

        // 取消监听属性变化
        KWinUtils::instance()->removeWindowPropertyMonitor(m_atom_deepin_no_titlebar);
        KWinUtils::instance()->removeWindowPropertyMonitor(m_atom_deepin_force_decorate);
        KWinUtils::instance()->removeWindowPropertyMonitor(m_atom_deepin_scissor_window);
        KWinUtils::instance()->removeWindowPropertyMonitor(m_atom_net_wm_window_type);
    }
#endif

    if (!active) {
        ChameleonShadow::instance()->clearCache();
        clearX11ShadowCache();
    }

    enforcePropertiesForWindows(active);

    emit activatedChanged(active);
}

enum ShadowElements {
    ShadowElementTop,
    ShadowElementTopRight,
    ShadowElementRight,
    ShadowElementBottomRight,
    ShadowElementBottom,
    ShadowElementBottomLeft,
    ShadowElementLeft,
    ShadowElementTopLeft,
    ShadowElementsCount,
    ShadowTopOffse = ShadowElementsCount,
    ShadowRightOffse = ShadowTopOffse + 1,
    ShadowBottomOffse = ShadowTopOffse + 2,
    ShadowLeftOffse = ShadowTopOffse + 3
};

class ShadowImage
{
public:
    ShadowImage(const QImage &image)
    {
        pixmap = XCreatePixmap(QX11Info::display(), QX11Info::appRootWindow(), image.width(), image.height(), image.depth());
        auto xcb_conn = QX11Info::connection();
        xcb_gcontext_t gc = xcb_generate_id(xcb_conn);
        xcb_create_gc(xcb_conn, gc, pixmap, 0, 0x0);
        xcb_put_image(xcb_conn, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap, gc,
                      image.width(), image.height(), 0, 0,
                      0, image.depth(),
                      image.byteCount(), image.constBits());
        xcb_free_gc(xcb_conn, gc);
    }

    ~ShadowImage() {
        XFreePixmap(QX11Info::display(), pixmap);
    }

    Pixmap pixmap;
};

class X11Shadow {
public:
    X11Shadow() {

    }

    ~X11Shadow() {
        if (valid)
            clear();
    }

    bool isValid() const {
        return valid;
    }

    void init(const QSharedPointer<KDecoration2::DecorationShadow> &shadow) {
        if (valid)
            return;

        QList<QRect> shadow_rect_list {
            shadow->topGeometry(),
            shadow->topRightGeometry(),
            shadow->rightGeometry(),
            shadow->bottomRightGeometry(),
            shadow->bottomGeometry(),
            shadow->bottomLeftGeometry(),
            shadow->leftGeometry(),
            shadow->topLeftGeometry()
        };

        const QImage &shadow_image = shadow->shadow();

        for (int i = 0; i < ShadowElements::ShadowElementsCount; ++i) {
            const QRect &rect = shadow_rect_list[i];
            ShadowImage *window = new ShadowImage(shadow_image.copy(rect));

            shadowWindowList[i] = window;
        }

        shadowOffset << shadow->paddingTop()
                     << shadow->paddingRight()
                     << shadow->paddingBottom()
                     << shadow->paddingLeft();

        valid = true;
    }

    void clear() {
        valid = false;

        for (int i = 0; i < ShadowElements::ShadowElementsCount; ++i) {
            delete shadowWindowList[i];
        }
    }

    QVector<quint32> toX11ShadowProperty() const {
        QVector<quint32> xwinid_list;

        for (int i = 0; i < ShadowElements::ShadowElementsCount; ++i) {
            xwinid_list << shadowWindowList[i]->pixmap;
        }

        xwinid_list << shadowOffset;

        return xwinid_list;
    }

private:
    bool valid = false;
    QVector<quint32> shadowOffset;
    ShadowImage *shadowWindowList[ShadowElements::ShadowElementsCount];
};

void ChameleonConfig::buildKWinX11Shadow(QObject *window)
{
    bool force_decorate = window->property(DDE_FORCE_DECORATE).toBool();
    bool can_build_shadow = canForceSetBorder(window);

    // 应该强制为菜单类型的窗口创建阴影
    if (!force_decorate && window->property("popupMenu").toBool()) {
        // 设置了 GTK_FRAME_EXTENTS 的窗口不接管其阴影
        if (!window->property("clientSideDecorated").toBool())
            can_build_shadow = true;
    }

    if (can_build_shadow) {
        // 对于可以强制设置显示边框的窗口, "如果声明了边框强制修饰/当前有边框/无边框但透明"的窗口不创建阴影
        if (force_decorate
                || (window->property("noBorder").isValid() && !window->property("noBorder").toBool())
                || window->property("alpha").toBool()) {
            return;
        }
    } else if (!force_decorate) {
        return;
    }

    ChameleonWindowTheme *window_theme = buildWindowTheme(window);

    if (!window_theme->property("__connected_for_shadow").toBool()) {
        auto update = [window, this] {
            buildKWinX11Shadow(window);
        };

        connect(window_theme, &ChameleonWindowTheme::themeChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::shadowRadiusChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::shadowOffectChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::shadowColorChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::windowRadiusChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::borderColorChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::borderWidthChanged, this, update);
        connect(window_theme, &ChameleonWindowTheme::windowPixelRatioChanged, this, update);
        // 标记为已初始化信号链接
        window_theme->setProperty("__connected_for_shadow", true);
    }

    ShadowCacheType shadow_type;

    if (canForceSetBorder(window)) {
        shadow_type = window->property("active").toBool() ? ActiveType : InactiveType;
    } else {
        shadow_type = UnmanagedType;
    }

    auto theme_group = ChameleonTheme::instance()->themeConfig();
    ChameleonTheme::DecorationConfig theme_config;

    // 如果有效，应该使用窗口自定的主题
    if (window_theme->propertyIsValid(ChameleonWindowTheme::ThemeProperty)) {
        if (auto new_theme = ChameleonTheme::instance()->loadTheme(window_theme->theme())) {
            theme_group = new_theme;
        }
    }

    switch (shadow_type) {
    case ActiveType:
        theme_config = theme_group->normal.decoration;
        break;
    case InactiveType:
        theme_config = theme_group->inactive.decoration;
        break;
    case UnmanagedType:
        theme_config = theme_group->unmanaged.decoration;
    default:
        break;
    }

    qreal scale = window_theme->windowPixelRatio();

    if (window_theme->propertyIsValid(ChameleonWindowTheme::WindowRadiusProperty)) {
        theme_config.windowRadius = window_theme->windowRadius();
        scale = 1.0; // 窗口自定义的值不受缩放控制
    }

    if (window_theme->propertyIsValid(ChameleonWindowTheme::BorderWidthProperty)) {
        theme_config.borderWidth = window_theme->borderWidth();
    }

    if (window_theme->propertyIsValid(ChameleonWindowTheme::BorderColorProperty)) {
        theme_config.borderColor = window_theme->borderColor();
    }

    if (window_theme->propertyIsValid(ChameleonWindowTheme::ShadowColorProperty)) {
        theme_config.shadowColor = window_theme->shadowColor();
    }

    if (window_theme->propertyIsValid(ChameleonWindowTheme::ShadowOffsetProperty)) {
        theme_config.shadowOffset = window_theme->shadowOffset();
    }

    if (window_theme->propertyIsValid(ChameleonWindowTheme::ShadowRadiusProperty)) {
        theme_config.shadowRadius = window_theme->shadowRadius();
    }

    if (!force_decorate) {
        // 无边框
        theme_config.borderWidth = 0;
    }

    const QString &shadow_key = ChameleonShadow::buildShadowCacheKey(&theme_config, scale);
    X11Shadow *shadow = m_x11ShadowCache.value(shadow_key);

    if (!shadow && QX11Info::isPlatformX11()) {
        auto s = ChameleonShadow::instance()->getShadow(&theme_config, scale);

        if (s) {
            shadow = new X11Shadow();
            shadow->init(s);
            m_x11ShadowCache[shadow_key] = shadow;
        }
    }

    if (!shadow) {
        return;
    }

    auto property_data = shadow->toX11ShadowProperty();

    if (window->property("shaped").toBool()) {
        KWin::EffectWindow *effect = nullptr;

        effect = window->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);

        if (effect) {
#if !defined(KWIN_VERSION) || KWIN_VERSION < KWIN_VERSION_CHECK(5, 23, 4, 0)
            QRect shape_rect = effect->geometry();
#else
            QRect shape_rect = effect->clientGeometry();
#endif
            const QRect window_rect(QPoint(0, 0), window->property("size").toSize());

            // 减去窗口的shape区域
            if (shape_rect.isValid() && window_rect.isValid()) {
                property_data[ShadowTopOffse] -= shape_rect.top();
                property_data[ShadowRightOffse] -= window_rect.right() - shape_rect.right();
                property_data[ShadowBottomOffse] -= window_rect.bottom() - shape_rect.bottom();
                property_data[ShadowLeftOffse] -= shape_rect.left();
            }
        }
    }

    KWinUtils::setWindowProperty(window, m_atom_kde_net_wm_shadow,
                                 XCB_ATOM_CARDINAL, 32,
                                 QByteArray((char*)property_data.constData(), property_data.size() * 4));
}

void ChameleonConfig::buildKWinX11ShadowDelay(QObject *client, int delay)
{
    // 窗口已经进入等待更新阴影的状态时，可以直接忽略本次请求
    if (client->property("__dde__delay_build_shadow").toBool())
        return;

    QPointer<ChameleonConfig> self(this);
    auto buildClientShadow = [client, self] {
        if (!self) {
            return;
        }

        // 清理标记的属性
        client->setProperty("__dde__delay_build_shadow", QVariant());
        self->buildKWinX11Shadow(client);
    };

    // 标记为正在等待更新阴影
    client->setProperty("__dde__delay_build_shadow", true);
    QTimer::singleShot(delay, client, buildClientShadow);
}

void ChameleonConfig::buildKWinX11ShadowForNoBorderWindows()
{
    for (QObject *client : KWinUtils::clientList()) {
        buildKWinX11Shadow(client);
    }

    for (QObject *client : KWinUtils::unmanagedList()) {
        buildKWinX11Shadow(client);
    }
}

void ChameleonConfig::clearKWinX11ShadowForWindows()
{
    for (const QObject *client : KWinUtils::clientList()) {
        KWinUtils::setWindowProperty(client, m_atom_kde_net_wm_shadow, 0, 0, QByteArray());
    }
}

void ChameleonConfig::clearX11ShadowCache()
{
    qDeleteAll(m_x11ShadowCache);
    m_x11ShadowCache.clear();
}

void ChameleonConfig::enforceWindowProperties(QObject *client)
{
    updateClientNoBorder(client, false);
    updateClientClipPath(client);
    updateClientWindowRadius(client);
}

void ChameleonConfig::enforcePropertiesForWindows(bool enable)
{
#ifndef DISBLE_DDE_KWIN_XCB
    for (QObject *client : KWinUtils::clientList()) {
        if (enable) {
            enforceWindowProperties(client);
        } else {
            // 重置窗口的noborder状态
            KWinUtils::instance()->clientCheckNoBorder(client);

            // 清理窗口的裁剪
            if (KWin::EffectWindow *effect = findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly)) {
                effect->setData(WindowClipPathRole, QVariant());
            }
        }
    }

    for (QObject *unmanaged : KWinUtils::unmanagedList()) {
        if (enable) {
            enforceWindowProperties(unmanaged);
        } else {
            // 清理窗口的裁剪
            if (KWin::EffectWindow *effect = findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly)) {
                effect->setData(WindowClipPathRole, QVariant());
            }
        }
    }
#endif
}

bool ChameleonConfig::setWindowOverrideType(QObject *client, bool enable)
{
#ifndef DISBLE_DDE_KWIN_XCB
    // 曾经不是override类型的窗口不允许设置为override类型
    if (enable && !client->property("__dde__override_type").toBool()) {
        return false;
    }

    const QByteArray &data = KWinUtils::instance()->readWindowProperty(client, m_atom_net_wm_window_type, XCB_ATOM_ATOM);

    if (data.isEmpty()) {
        return false;
    }

    QVector<xcb_atom_t> atom_list;
    const xcb_atom_t *atoms = reinterpret_cast<const xcb_atom_t*>(data.constData());

    for (int i = 0; i < data.size() / sizeof(xcb_atom_t) * sizeof(char); ++i) {
        atom_list.append(atoms[i]);
    }

    static xcb_atom_t _KDE_NET_WM_WINDOW_TYPE_OVERRIDE = KWinUtils::instance()->getXcbAtom("_KDE_NET_WM_WINDOW_TYPE_OVERRIDE", true);

    if (!enable) {
        // 移除override窗口属性，并且重设给window
        if (atom_list.removeAll(_KDE_NET_WM_WINDOW_TYPE_OVERRIDE)) {
            const QByteArray data((const char*)atom_list.constData(), atom_list.size() * sizeof(xcb_atom_t));
            KWinUtils::instance()->setWindowProperty(client, m_atom_net_wm_window_type, XCB_ATOM_ATOM, 32, data);

            if (QX11Info::isPlatformX11())
                xcb_flush(QX11Info::connection());
            // 标记窗口曾经为override类型，用于重设窗口类型
            client->setProperty("__dde__override_type", true);

            return true;
        }
    } else if (!atom_list.contains(_KDE_NET_WM_WINDOW_TYPE_OVERRIDE)) {
        atom_list.append(_KDE_NET_WM_WINDOW_TYPE_OVERRIDE);
        const QByteArray data((const char*)atom_list.constData(), atom_list.size() * sizeof(xcb_atom_t));
        KWinUtils::instance()->setWindowProperty(client, m_atom_net_wm_window_type, XCB_ATOM_ATOM, 32, data);

        if (QX11Info::isPlatformX11())
            xcb_flush(QX11Info::connection());
        // 取消窗口override类型标记
        client->setProperty("__dde__override_type", QVariant());

        return true;
    }
#endif
    return false;
}
