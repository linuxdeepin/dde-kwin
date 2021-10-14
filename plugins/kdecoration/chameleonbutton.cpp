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

#include <QPainter>
#include <QDebug>
#include <QX11Info>

ChameleonButton::ChameleonButton(KDecoration2::DecorationButtonType type, const QPointer<KDecoration2::Decoration> &decoration, QObject *parent)
    : KDecoration2::DecorationButton(type, decoration, parent)
{
    auto c = decoration->client().data();

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

