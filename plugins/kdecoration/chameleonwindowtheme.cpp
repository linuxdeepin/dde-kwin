// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "chameleonwindowtheme.h"
#include "chameleontheme.h"
#ifndef DISBLE_DDE_KWIN_XCB
#include "kwinutils.h"
#endif

#include <QDebug>
#include <QGuiApplication>
#include <QX11Info>

ChameleonWindowTheme::ChameleonWindowTheme(QObject *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
{
    if (!window)
        return;

#ifndef DISBLE_DDE_KWIN_XCB
    bool ok = false;
    quint32 window_id = KWinUtils::instance()->getWindowId(window, &ok);

    if (!ok)
        return;

    // 将ChameleonWindowTheme对象的属性绑定到对应x11窗口的settings属性
    ok = KWinUtils::instance()->buildNativeSettings(this, window_id);
    Q_UNUSED(ok);
#endif

    updateScreen();
}

ChameleonWindowTheme::PropertyFlags ChameleonWindowTheme::validProperties() const
{
    return m_validProperties;
}

bool ChameleonWindowTheme::propertyIsValid(ChameleonWindowTheme::PropertyFlag p) const
{
    return m_validProperties.testFlag(p);
}

QString ChameleonWindowTheme::theme() const
{
    return property("theme").toString();
}

QPointF ChameleonWindowTheme::windowRadius() const
{
    if(!QX11Info::isPlatformX11()) {
        return m_windowRadius;
    }
    return ChameleonTheme::takePos(property("windowRadius"), QPointF(0.0, 0.0));
}

void ChameleonWindowTheme::setWindowRadius(const QPointF value) {
    this->m_windowRadius = value;
}

qreal ChameleonWindowTheme::borderWidth() const
{
    return property("borderWidth").toDouble();
}

QColor ChameleonWindowTheme::borderColor() const
{
    return qvariant_cast<QColor>(property("borderColor"));
}

qreal ChameleonWindowTheme::shadowRadius() const
{
    return property("shadowRadius").toDouble();
}

QPointF ChameleonWindowTheme::shadowOffset() const
{
    return ChameleonTheme::takePos(property("shadowOffect"), QPointF(0.0, 0.0));
}

QColor ChameleonWindowTheme::shadowColor() const
{
    return qvariant_cast<QColor>(property("shadowColor"));
}

QMarginsF ChameleonWindowTheme::mouseInputAreaMargins() const
{
    return ChameleonTheme::takeMargins(property("mouseInputAreaMargins"), QMarginsF(0, 0, 0, 0));
}

qreal ChameleonWindowTheme::windowPixelRatio() const
{
    return m_validProperties.testFlag(WindowPixelRatioProperty) ? property("windowPixelRatio").toDouble() : m_windowPixelRatio ;
}

void ChameleonWindowTheme::setValidProperties(qint64 validProperties)
{
    if (m_validProperties == validProperties)
        return;

    PropertyFlags p = PropertyFlag(validProperties);

    if (m_validProperties.testFlag(WindowPixelRatioProperty) && !p.testFlag(WindowPixelRatioProperty)) {
        emit windowPixelRatioChanged();
    }

    m_validProperties = p;
    emit validPropertiesChanged(m_validProperties);
}

void ChameleonWindowTheme::updateScreen()
{
    QScreen *screen = nullptr;

    if (m_window) {
        bool ok = false;
        int screen_index = m_window->property("screen").toInt(&ok);

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
        disconnect(m_screen, &QScreen::logicalDotsPerInchChanged, this, &ChameleonWindowTheme::updateScreenScale);
        disconnect(m_screen, &QScreen::destroyed, this, &ChameleonWindowTheme::updateScreen);
    }

    m_screen = screen;

    connect(m_screen, &QScreen::logicalDotsPerInchChanged, this, &ChameleonWindowTheme::updateScreenScale);
    connect(m_screen, &QScreen::destroyed, this, &ChameleonWindowTheme::updateScreen);

    updateScreenScale();
}

void ChameleonWindowTheme::updateScreenScale()
{
    if (m_screen == nullptr)
        return;

    qreal scale = m_screen->logicalDotsPerInch() / 96.0f;

    if (qFuzzyCompare(scale, m_windowPixelRatio))
        return;

    m_windowPixelRatio = scale;

    if (!m_validProperties.testFlag(WindowPixelRatioProperty)) {
        emit windowPixelRatioChanged();
    }
}
