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

#ifndef DISBLE_DDE_KWIN_XCB
#include "kwinutils.h"
#endif

#include <kwineffects.h>

#include <KConfig>
#include <KConfigGroup>
#include <KDecoration2/Decoration>
#include <KDecoration2/DecoratedClient>

#include <QPainter>
#include <QDebug>
#include <QX11Info>
#include <QGuiApplication>

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

void ChameleonConfig::onClientAdded(KWin::Client *client)
{
    QObject *c = reinterpret_cast<QObject*>(client);

    connect(c, SIGNAL(activeChanged()), this, SLOT(updateClientX11Shadow()));
    connect(c, SIGNAL(hasAlphaChanged()), this, SLOT(updateClientX11Shadow()));
    connect(c, SIGNAL(geometryShapeChanged(KWin::Toplevel*, const QRect&)), this, SLOT(updateClientX11Shadow()));
    connect(c, SIGNAL(shapedChanged()), this, SLOT(updateClientX11Shadow()));

    enforceWindowProperties(c);
    buildKWinX11Shadow(c);
}

void ChameleonConfig::onUnmanagedAdded(KWin::Unmanaged *client)
{
    QObject *c = reinterpret_cast<QObject*>(client);

    connect(c, SIGNAL(geometryShapeChanged(KWin::Toplevel*, const QRect&)), this, SLOT(updateClientX11Shadow()));
    connect(c, SIGNAL(shapedChanged()), this, SLOT(updateClientX11Shadow()));

    enforceWindowProperties(c);
    buildKWinX11Shadow(c);
}

void ChameleonConfig::onCompositingToggled(bool active)
{
#ifndef DISBLE_DDE_KWIN_XCB
    if (active && isActivated()) {
        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_scissor_window);

        // 需要重设窗口的clip path特效数据
        for (QObject *client : KWinUtils::clientList()) {
            updateClientClipPath(client);
            updateClientWindowRadius(client);
        }
    } else {
        KWinUtils::instance()->removeSupportedProperty(m_atom_deepin_scissor_window);
    }
#endif
}

void ChameleonConfig::onWindowPropertyChanged(quint32 windowId, quint32 atom)
{
#ifndef DISBLE_DDE_KWIN_XCB
    if (atom == m_atom_deepin_no_titlebar) {
        emit windowNoTitlebarPropertyChanged(windowId);
    } else if (atom == m_atom_deepin_force_decorate) {
        updateClientNoBorder(KWinUtils::instance()->findClient(KWinUtils::Predicate::WindowMatch, windowId));

        emit windowForceDecoratePropertyChanged(windowId);
    } else if (atom == m_atom_deepin_scissor_window) {
        updateClientClipPath(KWinUtils::instance()->findClient(KWinUtils::Predicate::WindowMatch, windowId));

        emit windowScissorWindowPropertyChanged(windowId);
    } else if (atom == m_atom_net_wm_window_type) {
        QObject *client = KWinUtils::instance()->findClient(KWinUtils::Predicate::WindowMatch, windowId);

        if (!client)
            return;

        emit windowTypeChanged(client);

        bool force_decorate = client->property(DDE_FORCE_DECORATE).toBool();

        if (!force_decorate)
            return;

        setWindowOverrideType(client, false);
    }
#endif
}

void ChameleonConfig::updateWindowNoBorderProperty(QObject *window)
{
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

void ChameleonConfig::updateClientX11Shadow()
{
    buildKWinX11Shadow(QObject::sender());
}

void ChameleonConfig::updateClientNoBorder(QObject *client, bool allowReset)
{
#ifndef DISBLE_DDE_KWIN_XCB
    // 不要直接调用updateClientNoBorder
    const QByteArray &force_decorate = KWinUtils::instance()->readWindowProperty(client, m_atom_deepin_force_decorate, XCB_ATOM_CARDINAL);
    bool managed = client->property("managed").toBool();

    if (!force_decorate.isEmpty() && force_decorate.at(0)) {
        // 对于未被管理的窗口，必处于noBorder状态
        if (managed) {
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
    ChameleonWindowTheme *window_theme = window->findChild<ChameleonWindowTheme*>(QString(), Qt::FindDirectChildrenOnly);

    // 构建窗口主题设置对象
    if (!window_theme) {
        window_theme = new ChameleonWindowTheme(window, window);
    }

    return window_theme;
}

// 针对未被管理的窗口，且设置了强制开启窗口修饰时，更新其窗口圆角状态
void ChameleonConfig::updateClientWindowRadius(QObject *client)
{
    if (client->property("managed").toBool()) {
        return;
    }

    if (!client->property(DDE_FORCE_DECORATE).toBool()) {
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
            window_radius = config_group->unmanaged.decoration.windowRadius;
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
#ifndef DISBLE_DDE_KWIN_XCB
    KWin::EffectWindow *effect = client->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);

    if (!effect)
        return;

    QPainterPath path;
    const QByteArray &clip_data = KWinUtils::instance()->readWindowProperty(client, m_atom_deepin_scissor_window, m_atom_deepin_scissor_window);

    if (!clip_data.isEmpty()) {
        QDataStream ds(clip_data);
        ds >> path;
    }

    if (path.isEmpty()) {
        effect->setData(WindowClipPathRole, QVariant());
    } else {
        effect->setData(WindowClipPathRole, QVariant::fromValue(path));
    }
#endif
}

void ChameleonConfig::init()
{
#ifndef DISBLE_DDE_KWIN_XCB
    connect(KWinUtils::workspace(), SIGNAL(configChanged()), this, SLOT(onConfigChanged()));
    connect(KWinUtils::workspace(), SIGNAL(clientAdded(KWin::Client*)), this, SLOT(onClientAdded(KWin::Client*)));
    connect(KWinUtils::workspace(), SIGNAL(unmanagedAdded(KWin::Unmanaged*)), this, SLOT(onUnmanagedAdded(KWin::Unmanaged*)));
    connect(KWinUtils::compositor(), SIGNAL(compositingToggled(bool)), this, SLOT(onCompositingToggled(bool)));
    connect(KWinUtils::instance(), &KWinUtils::windowPropertyChanged, this, &ChameleonConfig::onWindowPropertyChanged);
#endif

    // 初始化链接客户端的信号
    for (QObject *c : KWinUtils::instance()->clientList()) {
        connect(c, SIGNAL(activeChanged()), this, SLOT(updateClientX11Shadow()));
        connect(c, SIGNAL(hasAlphaChanged()), this, SLOT(updateClientX11Shadow()));
        connect(c, SIGNAL(geometryShapeChanged(KWin::Toplevel*, const QRect&)), this, SLOT(updateClientX11Shadow()));
        connect(c, SIGNAL(shapedChanged()), this, SLOT(updateClientX11Shadow()));
    }

    for (QObject *c : KWinUtils::instance()->unmanagedList()) {
        connect(c, SIGNAL(geometryShapeChanged(KWin::Toplevel*, const QRect&)), this, SLOT(updateClientX11Shadow()));
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
        if (KWinUtils::compositorIsActive())
            KWinUtils::instance()->addSupportedProperty(m_atom_deepin_scissor_window, false);

        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_chameleon, false);
        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_no_titlebar, false);
        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_force_decorate);

        // 监听属性变化
        KWinUtils::instance()->addWindowPropertyMonitor(m_atom_deepin_no_titlebar);
        KWinUtils::instance()->addWindowPropertyMonitor(m_atom_deepin_force_decorate);
        KWinUtils::instance()->addWindowPropertyMonitor(m_atom_deepin_scissor_window);
        KWinUtils::instance()->addWindowPropertyMonitor(m_atom_net_wm_window_type);
    } else {
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
                     // 不知道为什么，显示出的阴影效果中多偏移了1px，因此在此处减去1px的偏移
                     << shadow->paddingRight() - 1
                     << shadow->paddingBottom() - 1
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

    // 应该强制为菜单类型的窗口创建阴影
    if (window->property("popupMenu").toBool()) {
        force_decorate = true;
    }

    if (window->property("managed").toBool()) {
        // 有边框或者无边框但透明的窗口不创建阴影
        if (!window->property("noBorder").toBool() || window->property("alpha").toBool()) {
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
        // 标记为已初始化信号链接
        window_theme->setProperty("__connected_for_shadow", true);
    }

    ShadowCacheType shadow_type;

    if (window->property("managed").toBool()) {
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

    if (!shadow) {
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
            QRect shape_rect = effect->shape().boundingRect();
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
            xcb_flush(QX11Info::connection());
            // 标记窗口曾经为override类型，用于重设窗口类型
            client->setProperty("__dde__override_type", true);

            return true;
        }
    } else if (!atom_list.contains(_KDE_NET_WM_WINDOW_TYPE_OVERRIDE)) {
        atom_list.append(_KDE_NET_WM_WINDOW_TYPE_OVERRIDE);
        const QByteArray data((const char*)atom_list.constData(), atom_list.size() * sizeof(xcb_atom_t));
        KWinUtils::instance()->setWindowProperty(client, m_atom_net_wm_window_type, XCB_ATOM_ATOM, 32, data);
        xcb_flush(QX11Info::connection());
        // 取消窗口override类型标记
        client->setProperty("__dde__override_type", QVariant());

        return true;
    }
#endif
    return false;
}
