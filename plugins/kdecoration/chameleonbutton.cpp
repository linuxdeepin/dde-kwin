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
#include "chameleonbutton.h"
#include "chameleon.h"
#include "kwinutils.h"
#include <KDecoration2/DecoratedClient>
#include <KDecoration2/Decoration>
#include <QTimer>
#include <QHoverEvent>
#include <QPainter>
#include <QDebug>
#include <QX11Info>

#define LONG_PRESS_TIME     300
#define OUT_RELEASE_EVENT   100

ChameleonButton::ChameleonButton(KDecoration2::DecorationButtonType type, const QPointer<KDecoration2::Decoration> &decoration, QObject *parent)
    : KDecoration2::DecorationButton(type, decoration, parent)
{
    auto c = decoration->client().data();

    m_type = type;

    switch (type) {
    case KDecoration2::DecorationButtonType::Menu:
        break;
    case KDecoration2::DecorationButtonType::Minimize:
        setVisible(c->isMinimizeable());
        connect(c, &KDecoration2::DecoratedClient::minimizeableChanged, this, &ChameleonButton::setVisible);
        break;
    case KDecoration2::DecorationButtonType::Maximize:
        setVisible(c->isMaximizeable());
        connect(c, &KDecoration2::DecoratedClient::maximizeableChanged, this, &ChameleonButton::setVisible);
        break;
    case KDecoration2::DecorationButtonType::Close:
        setVisible(c->isCloseable());
        connect(c, &KDecoration2::DecoratedClient::closeableChanged, this, &ChameleonButton::setVisible);
        break;
    default: // 隐藏不支持的按钮
        setVisible(false);
        break;
    }

    if (m_type == KDecoration2::DecorationButtonType::Maximize) {
        connect(KWinUtils::compositor(), SIGNAL(compositingToggled(bool)), this, SLOT(onCompositorChanged(bool)));
    }
}

ChameleonButton::~ChameleonButton()
{
    if (m_pSplitMenu) {
        delete m_pSplitMenu;
        m_pSplitMenu = nullptr;
    }

}

KDecoration2::DecorationButton *ChameleonButton::create(KDecoration2::DecorationButtonType type, KDecoration2::Decoration *decoration, QObject *parent)
{
    return new ChameleonButton(type, decoration, parent);
}

void ChameleonButton::paint(QPainter *painter, const QRect &repaintRegion)
{
    Q_UNUSED(repaintRegion)

    Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());

    if (!decoration)
        return;

    const QRect &rect = geometry().toRect();

    painter->save();

    auto c = decoration->client().data();

    QIcon::Mode state = QIcon::Normal;

    if (!isEnabled()) {
        state = QIcon::Disabled;
    } else if (isPressed()) {
        state = QIcon::Selected;
    } else if (isHovered()) {
        state = QIcon::Active;
    }

    switch (type()) {
    case KDecoration2::DecorationButtonType::Menu: {
        c->icon().paint(painter, rect);
        break;
    }
    case KDecoration2::DecorationButtonType::ApplicationMenu: {
        decoration->menuIcon().paint(painter, rect, Qt::AlignCenter, state);
        break;
    }
    case KDecoration2::DecorationButtonType::Minimize: {
        decoration->minimizeIcon().paint(painter, rect, Qt::AlignCenter, state);
        break;
    }
    case KDecoration2::DecorationButtonType::Maximize: {
        if (isChecked())
            decoration->unmaximizeIcon().paint(painter, rect, Qt::AlignCenter, state);
        else
            decoration->maximizeIcon().paint(painter, rect, Qt::AlignCenter, state);
        break;
    }
    case KDecoration2::DecorationButtonType::Close: {
        decoration->closeIcon().paint(painter, rect, Qt::AlignCenter, state);
        break;
    }
    default:
        break;
    }

    painter->restore();
}

void ChameleonButton::hoverEnterEvent(QHoverEvent *event)
{
    if (KWinUtils::instance()->isCompositing()) {

        Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());
        if (decoration) {
            effect = decoration->effect();
            if (effect && !effect->isUserMove()) {
                KDecoration2::DecorationButton::hoverEnterEvent(event);

                if (!contains(event->posF())) {
                    return;
                }
                if (m_type == KDecoration2::DecorationButtonType::Maximize) {
                    if (KWinUtils::instance()->isCompositing()) {
                        if (!m_pSplitMenu) {
                            m_pSplitMenu = new ChameleonSplitMenu();
                            m_pSplitMenu->setEffect(decoration->client().data()->windowId());
                        }
                        if (m_pSplitMenu) {
                            m_pSplitMenu->stopTime();
                            m_pSplitMenu->Hide();
                        }
                        m_backgroundColor = decoration->getBackgroundColor();
                        if (!max_hover_timer) {
                            max_hover_timer = new QTimer();
                            max_hover_timer->setSingleShot(true);

                            connect(max_hover_timer, &QTimer::timeout, [this]{
                                if (m_pSplitMenu) {
                                    qreal x = effect->geometry().x();
                                    qreal y = effect->geometry().y();
                                    QPoint p(x + geometry().x(), y + geometry().height());
                                    m_pSplitMenu->setShowSt(true);
                                    m_pSplitMenu->stopTime();
                                    m_pSplitMenu->Show(p, m_backgroundColor);
                                }
                            });
                            max_hover_timer->start(500);
                        }
                        else {
                            max_hover_timer->start(500);
                        }
                    }
                }
            }
        }
    } else {
        KDecoration2::DecorationButton::hoverEnterEvent(event);
    }
}

void ChameleonButton::hoverLeaveEvent(QHoverEvent *event)
{
    if (KWinUtils::instance()->isCompositing()) {

        Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());
        if (decoration) {
            effect = decoration->effect();
            if (max_hover_timer && m_type == KDecoration2::DecorationButtonType::Maximize) {
                max_hover_timer->stop();
            }
            if (effect && !effect->isUserMove()) {
                KDecoration2::DecorationButton::hoverLeaveEvent(event);
                if (m_pSplitMenu && m_type == KDecoration2::DecorationButtonType::Maximize) {
                    m_pSplitMenu->setShowSt(false);
                    m_pSplitMenu->startTime();
                }
            }
        }
    } else {
        KDecoration2::DecorationButton::hoverLeaveEvent(event);
    }
}

void ChameleonButton::mousePressEvent(QMouseEvent *event)
{
    KDecoration2::DecorationButton::mousePressEvent(event);
    if (m_type == KDecoration2::DecorationButtonType::Maximize) {
        if (!max_timer) {
            max_timer = new QTimer();
            max_timer->setSingleShot(true);
            connect(max_timer, &QTimer::timeout, [this] {
                m_isMaxAvailble = false;
                Chameleon *decoration = qobject_cast<Chameleon*>(this->decoration());
                if (decoration) {
                    effect = decoration->effect();
                    if (m_pSplitMenu && effect) {
#if !defined(KWIN_VERSION) || KWIN_VERSION < KWIN_VERSION_CHECK(5, 23, 4, 0)
                        qreal x = effect->geometry().x();
                        qreal y = effect->geometry().y();
#else
                        qreal x = effect->clientGeometry().x();
                        qreal y = effect->clientGeometry().y();
#endif
                        QPoint p(x + geometry().x(), y + geometry().height());
                        m_pSplitMenu->setShowSt(true);
                        m_pSplitMenu->stopTime();
                        m_pSplitMenu->Show(p, m_backgroundColor);
                    }
                }
            });
            max_timer->start(LONG_PRESS_TIME);
        } else {
            max_timer->start(LONG_PRESS_TIME);
        }
    }
}

void ChameleonButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_type == KDecoration2::DecorationButtonType::Maximize) {
        if (max_timer) {
            max_timer->stop();
        }
        if (!m_isMaxAvailble) {
            event->setLocalPos(QPointF(event->localPos().x() - OUT_RELEASE_EVENT, event->localPos().y()));
        } else if (m_pSplitMenu) {
            m_pSplitMenu->setShowSt(false);
            m_pSplitMenu->Hide();
        }
    }
    KDecoration2::DecorationButton::mouseReleaseEvent(event);
    m_isMaxAvailble = true;
}

void ChameleonButton::onCompositorChanged(bool active)
{
    if (!active && m_pSplitMenu) {
        m_pSplitMenu->Hide();
    }
}

