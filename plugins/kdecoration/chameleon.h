// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CHAMELEON_H
#define CHAMELEON_H

#include "chameleontheme.h"

#include <KDecoration2/Decoration>
#include <KDecoration2/DecorationButtonGroup>

#include <kwineffects.h>

#include <QVariant>
#include <QDir>
#include <QSettings>
#include <QScreen>
#include <QPainterPath>
#include <KWayland/Server/ddeshell_interface.h>

class Settings;
class ChameleonWindowTheme;
class Chameleon : public KDecoration2::Decoration
{
    Q_OBJECT

public:
    explicit Chameleon(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    ~Chameleon();

    void paint(QPainter *painter, const QRect &repaintArea) override;

    const ChameleonTheme::Config *themeConfig() const;
    KWin::EffectWindow *effect() const;
    bool noTitleBar() const;

    qreal borderWidth() const;
    qreal titleBarHeight() const;
    qreal shadowRadius() const;
    QPointF shadowOffset() const;
    QPointF windowRadius() const;
    QMarginsF mouseInputAreaMargins() const;

    QColor shadowColor() const;
    QColor borderColor() const;
    QColor getBackgroundColor() const;

    QIcon menuIcon() const;
    QIcon minimizeIcon() const;
    QIcon maximizeIcon() const;
    QIcon unmaximizeIcon() const;
    QIcon closeIcon() const;

signals:
    void noTitleBarChanged(bool noTitleBar);
    void effectInitialized(KWin::EffectWindow *effect);

protected:
    void init() override;

private Q_SLOTS:
    void updateFont(QString updateType,QString val);

private:
    void initButtons();
    void updateButtonsGeometry();

    void updateTitleGeometry();

    void updateTheme();
    void updateConfig();
    void updateTitleBarArea();
    void updateBorderPath();
    void updateShadow();
    void updateMouseInputAreaMargins();

    void onClientWidthChanged();
    void onClientHeightChanged();
    void onNoTitlebarPropertyChanged(quint32 windowId);

    void onThemeWindowRadiusChanged();
    void onThemeBorderWidthChanged();
    void onThemeBorderColorChanged();
    void onThemeShadowRadiusChanged();
    void onThemeShadowOffsetChanged();

    bool windowNeedRadius() const;
    bool windowNeedBorder() const;

    QColor getTextColor() const;

    bool m_initialized = false;
    qint8 m_noTitleBar = -1;
    QObject *m_client = nullptr;

    QMarginsF m_titleBarAreaMargins;
    QPainterPath m_borderPath;
    ChameleonTheme::ConfigGroupPtr m_configGroup;
    ChameleonTheme::Config *m_config = nullptr;
    ChameleonWindowTheme *m_theme = nullptr;

    QString m_title;
    QRect m_titleArea;

    KDecoration2::DecorationButtonGroup *m_leftButtons = nullptr;
    KDecoration2::DecorationButtonGroup *m_rightButtons = nullptr;

    QPointer<KWin::EffectWindow> m_effect;
    QFont m_font;
    KWayland::Server::DDEShellSurfaceInterface * m_ddeShellSurface = nullptr;
};

#endif // CHAMELEON_H
