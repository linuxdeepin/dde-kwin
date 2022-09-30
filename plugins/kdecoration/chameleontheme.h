// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CHAMELEONTHEME_H
#define CHAMELEONTHEME_H

#include <QColor>
#include <QMarginsF>
#include <QDir>
#include <QSettings>
#include <qwindowdefs.h>
#include <QPointF>
#include <QIcon>

class ChameleonTheme
{
public:
    enum ThemeType {
        Light,
        Dark,
        ThemeTypeCount
    };

    enum ThemeClass {
        Decoration = 0x0001,
        TitleBar = 0x0002
    };
    Q_DECLARE_FLAGS(ThemeClassFlags, ThemeClass)

    struct DecorationConfig {
        qreal borderWidth;
        qreal shadowRadius;
        QPointF shadowOffset;
        QPointF windowRadius;
        QMarginsF mouseInputAreaMargins;

        QColor borderColor;
        QColor shadowColor;
    };

    struct TitleBarConfig {
        qreal height;
        Qt::Edge area;

        QColor textColor;
        QColor backgroundColor;

        QIcon menuIcon;
        QIcon minimizeIcon;
        QIcon maximizeIcon;
        QIcon unmaximizeIcon;
        QIcon closeIcon;
    };

    struct Config {
        DecorationConfig decoration;
        TitleBarConfig titlebar;
    };

    struct ConfigGroup : public QSharedData {
        Config normal;
        Config noAlphaNormal;
        Config inactive;
        Config noAlphaInactive;
        Config unmanaged;
        Config noAlphaUnmanaged;
    };

    static QPair<qreal, qreal> takePair(const QVariant &value, const QPair<qreal, qreal> defaultValue);
    static QMarginsF takeMargins(const QVariant &value, const QMarginsF &defaultValue);
    static QPointF takePos(const QVariant &value, const QPointF defaultValue);

    typedef QSharedDataPointer<ConfigGroup> ConfigGroupPtr;

    static ChameleonTheme *instance();
    static ConfigGroupPtr loadTheme(const QString &themeFullName, const QList<QDir> themeDirList);
    static ConfigGroupPtr loadTheme(ThemeType themeType, const QString &themeName, const QList<QDir> themeDirList);
    static ConfigGroupPtr getBaseConfig(ThemeType type, const QList<QDir> &themeDirList);
    static QString typeString(ThemeType type);
    static ThemeType typeFromString(const QString &type);

    QString theme() const;
    bool setTheme(const QString &themeFullName);
    bool setTheme(ThemeType type, const QString &theme);

    ConfigGroupPtr loadTheme(const QString &themeFullName);
    ConfigGroupPtr themeConfig() const;

protected:
    ChameleonTheme();
    ~ChameleonTheme();

private:
    QList<QDir> m_themeDirList;
    ThemeType m_type;
    QString m_theme;
    ConfigGroupPtr m_configGroup;
};

#endif // CHAMELEONTHEME_H
