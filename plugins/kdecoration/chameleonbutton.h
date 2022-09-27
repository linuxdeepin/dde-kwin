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
#ifndef CHAMELEONBUTTON_H
#define CHAMELEONBUTTON_H

#include <KDecoration2/DecorationButton>
#include "chameleonsplitmenu.h"

class ChameleonButton : KDecoration2::DecorationButton
{
    Q_OBJECT
public:
    explicit ChameleonButton(KDecoration2::DecorationButtonType type, const QPointer<KDecoration2::Decoration> &decoration, QObject *parent = nullptr);
    virtual ~ChameleonButton();

    static DecorationButton *create(KDecoration2::DecorationButtonType type, KDecoration2::Decoration *decoration, QObject *parent);

    virtual void hoverEnterEvent(QHoverEvent *event) override;
    virtual void hoverLeaveEvent(QHoverEvent *event) override;
protected slots:
    void onCompositorChanged(bool);

protected:
    void paint(QPainter *painter, const QRect &repaintRegion) override;

    KDecoration2::DecorationButtonType m_type;

    ChameleonSplitMenu *m_pSplitMenu = nullptr;
    QTimer *max_hover_timer = nullptr;

    QColor m_backgroundColor;
    KWin::EffectWindow *effect = nullptr;
};

#endif // CHAMELEONBUTTON_H
