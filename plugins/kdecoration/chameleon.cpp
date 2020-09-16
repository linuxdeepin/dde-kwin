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
#include "chameleonwindowtheme.h"
#ifndef DISBLE_DDE_KWIN_XCB
#include "kwinutils.h"
#endif

#include <KDecoration2/DecoratedClient>
#include <KDecoration2/DecorationSettings>
#include <KDecoration2/DecorationButtonGroup>

#include <KConfigCore/KConfig>
#include <KConfigCore/KConfigGroup>

#include <QObject>
#include <QPainter>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>

Q_DECLARE_METATYPE(QPainterPath)

Chameleon::Chameleon(QObject *parent, const QVariantList &args)
    : KDecoration2::Decoration(parent, args)
    , m_client(parent)
{

}

Chameleon::~Chameleon()
{
    if (KWin::EffectWindow *effect = this->effect()) {
        // 清理窗口特效的数据
        effect->setData(ChameleonConfig::WindowRadiusRole, QVariant());
        effect->setData(ChameleonConfig::WindowMaskTextureRole, QVariant());
    }
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
    m_theme = new ChameleonWindowTheme(m_client, this);

    updateTheme();

    connect(global_config, &ChameleonConfig::themeChanged, this, &Chameleon::updateTheme);
    connect(global_config, &ChameleonConfig::windowNoTitlebarPropertyChanged, this, &Chameleon::onNoTitlebarPropertyChanged);
    connect(settings().data(), &KDecoration2::DecorationSettings::alphaChannelSupportedChanged, this, &Chameleon::updateConfig);
    connect(c, &KDecoration2::DecoratedClient::activeChanged, this, &Chameleon::updateConfig);
    connect(c, &KDecoration2::DecoratedClient::widthChanged, this, &Chameleon::onClientWidthChanged);
    connect(c, &KDecoration2::DecoratedClient::heightChanged, this, &Chameleon::onClientHeightChanged);
    connect(c, &KDecoration2::DecoratedClient::maximizedChanged, this, &Chameleon::updateTitleBarArea, Qt::QueuedConnection);
    connect(c, &KDecoration2::DecoratedClient::adjacentScreenEdgesChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration2::DecoratedClient::maximizedHorizontallyChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration2::DecoratedClient::maximizedVerticallyChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration2::DecoratedClient::captionChanged, this, &Chameleon::updateTitleGeometry);
    connect(this, &Chameleon::noTitleBarChanged, this, &Chameleon::updateTitleBarArea, Qt::QueuedConnection);
    connect(m_theme, &ChameleonWindowTheme::themeChanged, this, &Chameleon::updateTheme);
    connect(m_theme, &ChameleonWindowTheme::windowRadiusChanged, this, &Chameleon::updateBorderPath);
    connect(m_theme, &ChameleonWindowTheme::windowRadiusChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::borderWidthChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::borderColorChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::shadowRadiusChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::shadowOffectChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::shadowColorChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::mouseInputAreaMarginsChanged, this, &Chameleon::updateMouseInputAreaMargins);
    connect(m_theme, &ChameleonWindowTheme::windowPixelRatioChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::windowPixelRatioChanged, this, &Chameleon::updateTitleBarArea);
    connect(qGuiApp, &QGuiApplication::fontChanged, this, &Chameleon::updateTitleGeometry);

    m_initialized = true;
}

void Chameleon::paint(QPainter *painter, const QRect &repaintArea)
{
    auto s = settings().data();

    if (!noTitleBar()) {
        if (windowNeedRadius()) {
            painter->setClipPath(m_borderPath);
        }

        painter->fillRect(titleBar() & repaintArea, getBackgroundColor());
        painter->setPen(getTextColor());
        painter->drawText(m_titleArea, Qt::AlignCenter, m_title);

        // draw all buttons
        m_leftButtons->paint(painter, repaintArea);
        m_rightButtons->paint(painter, repaintArea);
    }

    if (windowNeedBorder()) {
        qreal border_width = borderWidth();

        // 支持alpha通道时在阴影上绘制border
        if (!qIsNull(border_width)) {
            if (noTitleBar()) {
                painter->fillPath(m_borderPath, borderColor());
            } else {
                // 绘制path是沿着路径外圈绘制，所以此处应该+1才能把border绘制到窗口边缘
                painter->strokePath(m_borderPath, QPen(borderColor(), border_width + 1));
            }
        }
    }
}

const ChameleonTheme::Config *Chameleon::themeConfig() const
{
    return m_config;
}

KWin::EffectWindow *Chameleon::effect() const
{
    if (m_effect)
        return m_effect.data();

    if (!m_client)
        return nullptr;

    Chameleon *self = const_cast<Chameleon*>(this);
    self->m_effect = m_client->findChild<KWin::EffectWindow*>(QString(), Qt::FindDirectChildrenOnly);
    emit self->effectInitialized(m_effect.data());

    return m_effect.data();
}

bool Chameleon::noTitleBar() const
{
    if (m_noTitleBar < 0) {
        // 需要初始化
        const QByteArray &data = KWinUtils::instance()->readWindowProperty(client().data()->windowId(),
                                                                           ChameleonConfig::instance()->atomDeepinNoTitlebar(),
                                                                           XCB_ATOM_CARDINAL);

        qint8 no_titlebar = !data.isEmpty() && data.at(0);

        if (no_titlebar != m_noTitleBar) {
            const_cast<Chameleon*>(this)->m_noTitleBar = no_titlebar;

            emit const_cast<Chameleon*>(this)->noTitleBarChanged(m_noTitleBar);
        }
    }

    return m_noTitleBar;
}

qreal Chameleon::borderWidth() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::BorderWidthProperty)) {
        return m_theme->borderWidth();
    }

    return m_config->decoration.borderWidth;
}

qreal Chameleon::titleBarHeight() const
{
    return m_config->titlebar.height * m_theme->windowPixelRatio();
}

qreal Chameleon::shadowRadius() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowRadiusProperty)) {
        return m_theme->shadowRadius();
    }

    return m_config->decoration.shadowRadius;
}

QPointF Chameleon::shadowOffset() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowOffsetProperty)) {
        return m_theme->shadowOffset();
    }

    return m_config->decoration.shadowOffset;
}

QPointF Chameleon::windowRadius() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::WindowRadiusProperty)) {
        return m_theme->windowRadius();
    }

    return m_config->decoration.windowRadius * m_theme->windowPixelRatio();
}

QMarginsF Chameleon::mouseInputAreaMargins() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::MouseInputAreaMargins)) {
        return m_theme->mouseInputAreaMargins();
    }

    return m_config->decoration.mouseInputAreaMargins;
}

QColor Chameleon::shadowColor() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowColorProperty)) {
        return m_theme->shadowColor();
    }

    return m_config->decoration.shadowColor;
}

QColor Chameleon::borderColor() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::BorderColorProperty)) {
        return m_theme->borderColor();
    }

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
    connect(m_rightButtons, &KDecoration2::DecorationButtonGroup::geometryChanged,
            this, &Chameleon::updateTitleBarArea, Qt::QueuedConnection);
}

void Chameleon::updateButtonsGeometry()
{
    auto s = settings();
    auto c = client().data();

    // adjust button position
    const int bHeight = noTitleBar() ? 0 : titleBarHeight();
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

void Chameleon::updateTitleGeometry()
{
    auto s = settings();

    m_titleArea = titleBar();

    m_title = client().data()->caption();
    // 使用系统字体，不要使用 settings() 中的字体
    const QFontMetricsF fontMetrics(qGuiApp->font());
    int full_width = fontMetrics.width(m_title);

    if (m_config->titlebar.area == Qt::TopEdge || m_config->titlebar.area == Qt::BottomEdge) {
        int buttons_width = m_leftButtons->geometry().width() 
            + m_rightButtons->geometry().width() + 2 * s->smallSpacing();

        m_titleArea.setWidth(m_titleArea.width() - buttons_width);
        m_titleArea.moveLeft(m_leftButtons->geometry().right() + s->smallSpacing());

        if (full_width < (m_titleArea.right() - titleBar().center().x()) * 2) {
            m_titleArea.setWidth(full_width);
            m_titleArea.moveCenter(titleBar().center());

        } else if (full_width > m_titleArea.width()) {
            m_title = fontMetrics.elidedText(m_title,
                    Qt::ElideRight, qMax(m_titleArea.width(), m_titleArea.height()));
            m_titleArea.moveRight(m_rightButtons->geometry().left() + s->smallSpacing());

        } else {
            m_titleArea.setWidth(full_width);
            m_titleArea.moveRight(m_rightButtons->geometry().left() + s->smallSpacing());
        }

    } else  {
        int buttons_height = m_leftButtons->geometry().height() 
            + m_rightButtons->geometry().height() + 2 * s->smallSpacing();

        m_titleArea.setHeight(m_titleArea.height() - buttons_height);
        m_titleArea.moveTop(m_leftButtons->geometry().bottom() + s->smallSpacing());

        if (full_width < (m_titleArea.bottom() - titleBar().center().y()) * 2) {
            m_titleArea.setHeight(full_width);
            m_titleArea.moveCenter(titleBar().center());

        } else if (full_width > m_titleArea.height()) {
            m_title = fontMetrics.elidedText(m_title,
                    Qt::ElideRight, qMax(m_titleArea.width(), m_titleArea.height()));
            m_titleArea.moveBottom(m_rightButtons->geometry().top() + s->smallSpacing());

        } else {
            m_titleArea.setWidth(full_width);
            m_titleArea.moveBottom(m_rightButtons->geometry().top() + s->smallSpacing());
        }
    }

    update();
}

void Chameleon::updateTheme()
{
    QString theme_name;

    if (m_theme->propertyIsValid(ChameleonWindowTheme::ThemeProperty)) {
        theme_name = m_theme->theme();
    }

    ChameleonTheme::ConfigGroupPtr config_group;

    if (!theme_name.isEmpty()) {
        config_group = ChameleonTheme::instance()->loadTheme(theme_name);
    } else {
        config_group = ChameleonTheme::instance()->themeConfig();
    }

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

    updateMouseInputAreaMargins();
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

    qreal border_width = windowNeedBorder() ? borderWidth() : 0;
    qreal titlebar_height = noTitleBar() ? 0 : titleBarHeight();

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

void Chameleon::updateBorderPath()
{
    auto c = client().data();
    QRectF client_rect(0, 0, c->width(), c->height());
    client_rect += borders();
    client_rect.moveTopLeft(QPointF(0, 0));

    QPainterPath path;
    KWin::EffectWindow *effect = this->effect();

    if (windowNeedRadius()) {
        auto window_radius = windowRadius();
        path.addRoundedRect(client_rect, window_radius.x(), window_radius.y());

        if (effect) {
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
    } else {
        path.addRect(client_rect);

        if (effect) {
            // 清理已缓存的旧的窗口mask材质
            effect->setData(ChameleonConfig::WindowMaskTextureRole, QVariant());
            // 清理窗口圆角的设置
            effect->setData(ChameleonConfig::WindowRadiusRole, QVariant());
        }
    }

    m_borderPath = path;

    update();
}

void Chameleon::updateShadow()
{
    if (m_config && settings()->isAlphaChannelSupported()) {
        if (m_theme->validProperties() == ChameleonWindowTheme::PropertyFlags()) {
            return setShadow(ChameleonShadow::instance()->getShadow(&m_config->decoration, m_theme->windowPixelRatio()));
        }

        ChameleonTheme::DecorationConfig config = m_config->decoration;
        qreal scale = m_theme->windowPixelRatio();
        // 优先使用窗口自己设置的属性
        if (m_theme->propertyIsValid(ChameleonWindowTheme::WindowRadiusProperty)) {
            config.windowRadius = m_theme->windowRadius();
            // 这里的数据是已经缩放过的，因此scale值需要为1
            scale = 1.0;
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::BorderWidthProperty)) {
            config.borderWidth = m_theme->borderWidth();
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::BorderColorProperty)) {
            config.borderColor = m_theme->borderColor();
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowRadiusProperty)) {
            config.shadowRadius = m_theme->shadowRadius();
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowOffsetProperty)) {
            config.shadowOffset = m_theme->shadowOffset();
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowColorProperty)) {
            config.shadowColor = m_theme->shadowColor();
        }

        setShadow(ChameleonShadow::instance()->getShadow(&config, scale));
    }
}

void Chameleon::updateMouseInputAreaMargins()
{
    setResizeOnlyBorders(mouseInputAreaMargins().toMargins());
}

void Chameleon::onClientWidthChanged()
{
    updateTitleBarArea();
}

void Chameleon::onClientHeightChanged()
{
    updateTitleBarArea();
}

void Chameleon::onNoTitlebarPropertyChanged(quint32 windowId)
{
    if (client().data()->windowId() != windowId)
        return;

    // 标记为未初始化状态
    m_noTitleBar = -1;
}

bool Chameleon::windowNeedRadius() const
{
    auto c = client().data();
    return KWinUtils::instance()->isCompositing() && c->adjacentScreenEdges() == Qt::Edges();
}

bool Chameleon::windowNeedBorder() const
{
    if (client().data()->isMaximized())
        return false;

    // 开启窗口混成时可以在阴影图片中绘制窗口边框
    if (settings()->isAlphaChannelSupported()) {
        return false;
    }

    return true;
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
