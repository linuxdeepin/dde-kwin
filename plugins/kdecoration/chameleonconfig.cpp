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

#ifndef DISBLE_DDE_KWIN_XCB
#include "kwinutils.h"
#endif

#include <kwineffects.h>

#include <KConfig>
#include <KConfigGroup>
#include <QPainter>

#include <QDebug>
#include <QX11Info>
#include <QGuiApplication>

#include <xcb/xcb.h>
#include <X11/Xlib.h>

ChameleonConfig::ChameleonConfig(QObject *parent)
    : QObject(parent)
{
    if (KWinUtils::instance()->isInitialized()) {
        init();
    } else {
        connect(KWinUtils::instance(), &KWinUtils::initialized, this, &ChameleonConfig::init);
    }
}

ChameleonConfig *ChameleonConfig::instance()
{
    static ChameleonConfig *self = new ChameleonConfig();

    return self;
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

    int split = theme.indexOf("/");

    if (split > 0 && split < theme.size() - 1) {
        ChameleonTheme::instance()->setTheme(ChameleonTheme::typeFromString(theme.left(split)), theme.mid(split + 1));

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
    KConfig config("kwinrc", KConfig::SimpleConfig);
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

    buildKWinX11Shadow(c);
}

void ChameleonConfig::updateClientX11Shadow()
{
    buildKWinX11Shadow(QObject::sender());
}

void ChameleonConfig::init()
{
#ifndef DISBLE_DDE_KWIN_XCB
    connect(KWinUtils::workspace(), SIGNAL(configChanged()), this, SLOT(onConfigChanged()));
    connect(KWinUtils::workspace(), SIGNAL(clientAdded(KWin::Client*)), this, SLOT(onClientAdded(KWin::Client*)));

    m_atom_deepin_chameleon = KWinUtils::instance()->getXcbAtom(_DEEPIN_CHAMELEON, false);
#endif
    m_atom_kde_net_wm_shadow = KWinUtils::instance()->getXcbAtom(_KDE_NET_WM_SHADOW, false);

    onConfigChanged();
}

void ChameleonConfig::setActivated(bool active)
{
    if (m_activated == active)
        return;

    m_activated = active;

    if (active) {
#ifndef DISBLE_DDE_KWIN_XCB
        KWinUtils::instance()->addSupportedProperty(m_atom_deepin_chameleon);
#endif
    } else {
#ifndef DISBLE_DDE_KWIN_XCB
        KWinUtils::instance()->removeSupportedProperty(m_atom_deepin_chameleon);
#endif
        ChameleonShadow::instance()->clearCache();
    }

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

void ChameleonConfig::buildKWinX11Shadow(QObject *client)
{
    // 有边框或者透明的窗口不创建阴影
    if (!client->property("noBorder").toBool() || client->property("alpha").toBool()) {
        return;
    }

    bool is_active = client->property("active").toBool();

    X11Shadow *shadow = m_x11ShadowCache[is_active];

    if (!shadow) {
        auto theme_group = ChameleonTheme::instance()->getThemeConfig(0);
        ChameleonTheme::Config theme_config;

        theme_config = is_active ? theme_group->normal : theme_group->inactive;

        // 无边框
        theme_config.decoration.borderWidth = 0;

        auto s = ChameleonShadow::instance()->getShadow(&theme_config, 1);

        if (s) {
            shadow = new X11Shadow();
            shadow->init(s);
            m_x11ShadowCache[is_active] = shadow;
        }
    }

    if (!shadow) {
        return;
    }

    auto property_data = shadow->toX11ShadowProperty();

    if (client->property("shaped").toBool()) {
        KWin::EffectWindow *effect = nullptr;

        effect = client->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);

        if (effect) {
            QRect shape_rect = effect->shape().boundingRect();
            const QRect window_rect(QPoint(0, 0), client->property("size").toSize());

            // 减去窗口的shape区域
            if (shape_rect.isValid() && window_rect.isValid()) {
                property_data[ShadowTopOffse] -= shape_rect.top();
                property_data[ShadowRightOffse] -= window_rect.right() - shape_rect.right();
                property_data[ShadowBottomOffse] -= window_rect.bottom() - shape_rect.bottom();
                property_data[ShadowLeftOffse] -= shape_rect.left();
            }
        }
    }

    KWinUtils::setWindowProperty(client, m_atom_kde_net_wm_shadow,
                                 XCB_ATOM_CARDINAL, 32,
                                 QByteArray((char*)property_data.constData(), property_data.size() * 4));
}

// 为所有无边框的窗口设置阴影
void ChameleonConfig::buildKWinX11ShadowForNoBorderWindows()
{
    for (QObject *client : KWinUtils::clientList()) {
        onClientAdded(reinterpret_cast<KWin::Client*>(client));
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
    if (m_x11ShadowCache[0]) {
        delete m_x11ShadowCache[0];
        m_x11ShadowCache[0] = nullptr;
    }

    if (m_x11ShadowCache[1]) {
        delete m_x11ShadowCache[1];
        m_x11ShadowCache[1] = nullptr;
    }
}
