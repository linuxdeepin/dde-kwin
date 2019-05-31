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
#ifndef CHAMELEON_H
#define CHAMELEON_H

#include "chameleontheme.h"

#include <KDecoration2/Decoration>
#include <KDecoration2/DecorationButtonGroup>

#include <QVariant>
#include <QDir>
#include <QSettings>
#include <QScreen>

class Settings;
class Chameleon : public KDecoration2::Decoration
{
    Q_OBJECT

public:
    explicit Chameleon(QObject *parent = nullptr, const QVariantList &args = QVariantList());

    void paint(QPainter *painter, const QRect &repaintArea) override;

    qreal borderWidth() const;
    qreal titleBarHeight() const;
    qreal shadowRadius() const;
    QPointF shadowOffset() const;
    QPair<qreal, qreal> windowRadius() const;
    QMarginsF mouseInputAreaMargins() const;

    QColor shadowColor() const;

    QIcon menuIcon() const;
    QIcon minimizeIcon() const;
    QIcon maximizeIcon() const;
    QIcon unmaximizeIcon() const;
    QIcon closeIcon() const;

protected:
    void init() override;

private:
    void initButtons();
    void updateButtonsGeometry();

    void updateTitle();
    void updateTitleGeometry();

    void updateScreen();
    void updateScreenScale();

    void updateTheme();
    void updateConfig();
    void updateTitleBarArea();
    void updateBorderPath();
    void updateShadow();

    void onClientWidthChanged();
    void onClientHeightChanged();

    bool windowNeedRadius() const;

    QColor getTextColor() const;
    QColor getBackgroundColor() const;

    bool m_initialized = false;
    QObject *m_client = nullptr;
    QPointer<QScreen> m_screen;
    qreal m_scale = 1.0;

    QMarginsF m_titleBarAreaMargins;
    QPainterPath m_borderPath;
    ChameleonTheme::ConfigGroupPtr m_configGroup;
    const ChameleonTheme::Config *m_config = nullptr;

    QString m_title;
    QRect m_titleArea;

    KDecoration2::DecorationButtonGroup *m_leftButtons = nullptr;
    KDecoration2::DecorationButtonGroup *m_rightButtons = nullptr;
};

#endif // CHAMELEON_H
