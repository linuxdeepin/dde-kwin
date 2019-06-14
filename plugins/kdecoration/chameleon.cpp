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
#include "chameleon.h"
#include "chameleonshadow.h"
#include "chameleonbutton.h"
#include "chameleonconfig.h"
#ifndef DISBLE_DDE_KWIN_XCB
#include "kwinutils.h"
#endif

#include <KDecoration2/DecoratedClient>
#include <KDecoration2/DecorationSettings>
#include <KDecoration2/DecorationButtonGroup>

#include <KConfigCore/KConfig>
#include <KConfigCore/KConfigGroup>

#include <kwineffects.h>

#include <QObject>
#include <QPainter>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>

Chameleon::Chameleon(QObject *parent, const QVariantList &args)
    : KDecoration2::Decoration(parent, args)
    , m_client(parent)
{

}

void Chameleon::init()
{
    if (m_initialized)
        return;

    auto c = client().data();

#ifndef DISBLE_DDE_KWIN_XCB
    if (!m_client)
        m_client = KWinUtils::findClient(KWinUtils::Predicate::WindowMatch, c->windowId());
#endif
    initButtons();

    // 要放到updateTheme调用之前初始化此对象
    auto global_config = ChameleonConfig::instance();

    updateTheme();
    updateScreen();

    connect(global_config, &ChameleonConfig::themeChanged, this, &Chameleon::updateTheme);
    connect(settings().data(), &KDecoration2::DecorationSettings::alphaChannelSupportedChanged, this, &Chameleon::updateConfig);
    connect(c, &KDecoration2::DecoratedClient::activeChanged, this, &Chameleon::updateConfig);
    connect(c, &KDecoration2::DecoratedClient::widthChanged, this, &Chameleon::onClientWidthChanged);
    connect(c, &KDecoration2::DecoratedClient::heightChanged, this, &Chameleon::onClientHeightChanged);
    connect(c, &KDecoration2::DecoratedClient::maximizedChanged, this, &Chameleon::updateTitleBarArea);
    connect(c, &KDecoration2::DecoratedClient::adjacentScreenEdgesChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration2::DecoratedClient::maximizedHorizontallyChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration2::DecoratedClient::maximizedVerticallyChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration2::DecoratedClient::captionChanged, this, &Chameleon::updateTitle);

    m_initialized = true;
}

void Chameleon::paint(QPainter *painter, const QRect &repaintArea)
{
    auto s = settings().data();

    if (windowNeedRadius()) {
        painter->setClipPath(m_borderPath);
    }

    painter->fillRect(titleBar() & repaintArea, getBackgroundColor());
    painter->setFont(s->font());
    painter->setPen(getTextColor());
    painter->drawText(m_titleArea, Qt::AlignCenter | Qt::TextWrapAnywhere, m_title);

    // draw all buttons
    m_leftButtons->paint(painter, repaintArea);
    m_rightButtons->paint(painter, repaintArea);

    {
        qreal border_width = borderWidth();

        // 支持alpha通道时在阴影上绘制border
        if (!qIsNull(border_width) && !s->isAlphaChannelSupported()) {
            painter->setPen(QPen(m_config->decoration.borderColor, border_width, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
            painter->drawPath(m_borderPath);
        }
    }
}

const ChameleonTheme::Config *Chameleon::themeConfig() const
{
    return m_config;
}

qreal Chameleon::borderWidth() const
{
    return client().data()->isMaximized() ? 0 : m_config->decoration.borderWidth;
}

qreal Chameleon::titleBarHeight() const
{
    return m_config->titlebar.height * m_scale;
}

qreal Chameleon::shadowRadius() const
{
    return m_config->decoration.shadowRadius;
}

QPointF Chameleon::shadowOffset() const
{
    return m_config->decoration.shadowOffset;
}

QPair<qreal, qreal> Chameleon::windowRadius() const
{
    return qMakePair(m_config->decoration.windowRadius.first * m_scale,
                     m_config->decoration.windowRadius.second * m_scale);
}

QMarginsF Chameleon::mouseInputAreaMargins() const
{
    return m_config->decoration.mouseInputAreaMargins;
}

QColor Chameleon::shadowColor() const
{
    return m_config->decoration.shadowColor;
}

QColor Chameleon::borderColor() const
{
    return m_config->decoration.borderColor;
}

QIcon Chameleon::menuIcon() const
{
    return m_config->titlebar.menuIcon;
}

QIcon Chameleon::minimizeIcon() const
{
    return m_config->titlebar.minimizeIcon;
}

QIcon Chameleon::maximizeIcon() const
{
    return m_config->titlebar.maximizeIcon;
}

QIcon Chameleon::unmaximizeIcon() const
{
    return m_config->titlebar.unmaximizeIcon;
}

QIcon Chameleon::closeIcon() const
{
    return m_config->titlebar.closeIcon;
}

void Chameleon::initButtons()
{
    m_leftButtons = new KDecoration2::DecorationButtonGroup(KDecoration2::DecorationButtonGroup::Position::Left, this, &ChameleonButton::create);
    m_rightButtons = new KDecoration2::DecorationButtonGroup(KDecoration2::DecorationButtonGroup::Position::Right, this, &ChameleonButton::create);
}

void Chameleon::updateButtonsGeometry()
{
    auto s = settings();
    auto c = client().data();

    // adjust button position
    const int bHeight = titleBarHeight();
    const int bWidth = bHeight;

    foreach (const QPointer<KDecoration2::DecorationButton> &button, m_leftButtons->buttons() + m_rightButtons->buttons()) {
        button.data()->setGeometry(QRectF(QPoint(0, 0), QSizeF(bWidth, bHeight)));
    }

    // left buttons
    if (!m_leftButtons->buttons().isEmpty()) {
        // spacing
        m_leftButtons->setSpacing(0);

        // padding
        const int vPadding = 0;
        const int hPadding = s->smallSpacing();

        if (c->isMaximizedHorizontally()) {
            // add offsets on the side buttons, to preserve padding, but satisfy Fitts law
            m_leftButtons->buttons().front()->setGeometry(QRectF(QPoint(0, 0), QSizeF(bWidth + hPadding, bHeight)));
            m_leftButtons->setPos(QPointF(0, vPadding));
        } else {
            m_leftButtons->setPos(QPointF(hPadding + borderLeft(), vPadding));
        }
    }

    // right buttons
    if (!m_rightButtons->buttons().isEmpty()) {
        // spacing
        m_rightButtons->setSpacing(s->smallSpacing());

        // padding
        const int vPadding = 0;
        const int hPadding = 0;

        if (c->isMaximizedHorizontally()) {
            m_rightButtons->buttons().back()->setGeometry(QRectF(QPoint(0, 0), QSizeF(bWidth + hPadding, bHeight)));
            m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width(), vPadding));
        } else {
            m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width() - hPadding - borderRight(), vPadding));
        }
    }

    updateTitleGeometry();
}

void Chameleon::updateTitle()
{
    m_title = settings()->fontMetrics().elidedText(client().data()->caption(), Qt::ElideMiddle, qMax(m_titleArea.width(), m_titleArea.height()));

    update();
}

void Chameleon::updateTitleGeometry()
{
    auto s = settings();

    m_titleArea = titleBar();

    int buttons_width = m_leftButtons->geometry().width() + m_rightButtons->geometry().width() + 2 * s->smallSpacing();

    if (m_config->titlebar.area == Qt::TopEdge || m_config->titlebar.area == Qt::BottomEdge) {
        m_titleArea.setWidth(m_titleArea.width() - buttons_width);
    } else  {
        m_titleArea.setHeight(m_titleArea.height() - buttons_width);
    }

    m_titleArea.moveCenter(titleBar().center());

    updateTitle();
}

void Chameleon::updateScreen()
{
    QScreen *screen = nullptr;

    if (m_client) {
        bool ok = false;
        int screen_index = m_client->property("screen").toInt(&ok);

        if (ok) {
            screen = qGuiApp->screens().value(screen_index);
        }
    }

    if (!screen) {
        screen = qGuiApp->primaryScreen();
    }

    if (m_screen == screen) {
        return;
    }

    if (m_screen) {
        disconnect(m_screen, &QScreen::logicalDotsPerInchChanged, this, &Chameleon::updateScreenScale);
        disconnect(m_screen, &QScreen::destroyed, this, &Chameleon::updateScreen);
    }

    m_screen = screen;

    connect(m_screen, &QScreen::logicalDotsPerInchChanged, this, &Chameleon::updateScreenScale);
    connect(m_screen, &QScreen::destroyed, this, &Chameleon::updateScreen);

    updateScreenScale();
}

void Chameleon::updateScreenScale()
{
    qreal scale = m_screen->logicalDotsPerInch() / 96.0f;

    if (qFuzzyCompare(scale, m_scale))
        return;

    m_scale = scale;

    updateTitleBarArea();
    updateShadow();
    update();
}

void Chameleon::updateTheme()
{
    auto c = client().data();
    auto config_group = ChameleonTheme::instance()->getThemeConfig(c->windowId());

    if (m_configGroup == config_group) {
        return;
    }

    m_configGroup = config_group;
    updateConfig();
}

void Chameleon::updateConfig()
{
    auto c = client().data();

    bool active = c->isActive();
    bool hasAlpha = settings()->isAlphaChannelSupported();

    if (hasAlpha) {
        m_config = active ? &m_configGroup->normal : &m_configGroup->inactive;
    } else {
        m_config = active ? &m_configGroup->noAlphaNormal : &m_configGroup->noAlphaInactive;
    }

    setResizeOnlyBorders(m_config->decoration.mouseInputAreaMargins.toMargins());

    updateTitleBarArea();
    updateShadow();
    update();
}

void Chameleon::updateTitleBarArea()
{
    auto c = client().data();

    m_titleBarAreaMargins.setLeft(0);
    m_titleBarAreaMargins.setTop(0);
    m_titleBarAreaMargins.setRight(0);
    m_titleBarAreaMargins.setBottom(0);

    // 支持alpha通道时在阴影上绘制边框，因此不需要关心边框宽度
    qreal border_width = settings()->isAlphaChannelSupported() ? 0 : borderWidth();
    qreal titlebar_height = titleBarHeight();

    switch (m_config->titlebar.area) {
    case Qt::LeftEdge:
        m_titleBarAreaMargins.setLeft(titlebar_height);
        setTitleBar(QRect(border_width, border_width, titlebar_height, c->height()));
        setBorders(QMargins(border_width + titlebar_height, border_width,
                            border_width, border_width));
        break;
    case Qt::RightEdge:
        m_titleBarAreaMargins.setRight(titlebar_height);
        setTitleBar(QRect(border_width + c->width() - titlebar_height, border_width,
                          titlebar_height, c->height()));
        setBorders(QMargins(border_width, border_width,
                            border_width + titlebar_height, border_width));
        break;
    case Qt::TopEdge:
        m_titleBarAreaMargins.setTop(titlebar_height);
        setTitleBar(QRect(border_width, border_width,
                          c->width(), titlebar_height));
        setBorders(QMargins(border_width, border_width + titlebar_height,
                            border_width, border_width));
        break;
    case Qt::BottomEdge:
        m_titleBarAreaMargins.setBottom(titlebar_height);
        setTitleBar(QRect(border_width, border_width + c->height() - titlebar_height,
                          c->width(), titlebar_height));
        setBorders(QMargins(border_width, border_width,
                            border_width, border_width + titlebar_height));
        break;
    default:
        return;
    }

    updateBorderPath();
    updateButtonsGeometry();
}

enum EffectDataRole {
    BaseRole = KWin::DataRole::LanczosCacheRole + 100,
    WindowRadiusRole = BaseRole + 1,
    WindowClipPathRole = BaseRole + 2,
    WindowMaskTextureRole = BaseRole + 3
};

void Chameleon::updateBorderPath()
{
    auto c = client().data();
    QRectF client_rect(0, 0, c->width(), c->height());
    client_rect += borders();
    client_rect.moveTopLeft(QPointF(0, 0));

    QPainterPath path;
    KWin::EffectWindow *effect = nullptr;

    if (m_client) {
        effect = m_client->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);
    }

    if (windowNeedRadius()) {
        auto window_radius = windowRadius();
        path.addRoundedRect(client_rect, window_radius.first, window_radius.second);

        if (effect) {
            const QVariant &effect_window_radius = effect->data(EffectDataRole::WindowRadiusRole);
            bool need_update = true;

            if (effect_window_radius.isValid()) {
                auto old_window_radius = qvariant_cast<QPair<qreal, qreal>>(effect_window_radius);

                if (old_window_radius == window_radius) {
                    need_update = false;
                }
            }

            if (need_update) {
                // 清理已缓存的旧的窗口mask材质
                effect->setData(EffectDataRole::WindowMaskTextureRole, QVariant());
                // 设置新的窗口圆角
                effect->setData(EffectDataRole::WindowRadiusRole, QVariant::fromValue(window_radius));
            }
        }
    } else {
        path.addRect(client_rect);

        if (effect) {
            // 清理已缓存的旧的窗口mask材质
            effect->setData(EffectDataRole::WindowMaskTextureRole, QVariant());
            // 清理窗口圆角的设置
            effect->setData(EffectDataRole::WindowRadiusRole, QVariant());
        }
    }

    m_borderPath = path;

    update();
}

void Chameleon::updateShadow()
{
    if (m_config && settings()->isAlphaChannelSupported()) {
        setShadow(ChameleonShadow::instance()->getShadow(m_config, m_scale));
    }
}

void Chameleon::onClientWidthChanged()
{
    updateTitleBarArea();
}

void Chameleon::onClientHeightChanged()
{
    updateTitleBarArea();
}

bool Chameleon::windowNeedRadius() const
{
    auto c = client().data();

    return c->adjacentScreenEdges() == Qt::Edges();
}

QColor Chameleon::getTextColor() const
{
    if (m_config->titlebar.textColor.isValid())
        return m_config->titlebar.textColor;

    auto c = client().data();

    return  c->color(c->isActive() ? KDecoration2::ColorGroup::Active : KDecoration2::ColorGroup::Inactive, KDecoration2::ColorRole::Foreground);
}

QColor Chameleon::getBackgroundColor() const
{
    if (m_config->titlebar.backgroundColor.isValid())
        return m_config->titlebar.backgroundColor;

    auto c = client().data();

    return c->color(c->isActive() ? KDecoration2::ColorGroup::Active : KDecoration2::ColorGroup::Inactive, KDecoration2::ColorRole::TitleBar);
}
