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
        QPair<qreal, qreal> windowRadius;
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
    };

    typedef QExplicitlySharedDataPointer<const ConfigGroup> ConfigGroupPtr;

    static ChameleonTheme *instance();
    static ConfigGroupPtr loadTheme(ThemeType themeType, const QString &themeName, const QList<QDir> themeDirList);
    static ConfigGroupPtr getBaseConfig(ThemeType type, const QList<QDir> &themeDirList);
    static QString typeString(ThemeType type);
    static ThemeType typeFromString(const QString &type);

    QString theme() const;
    bool setTheme(ThemeType type, const QString &theme);

    ConfigGroupPtr getThemeConfig(WId windowId) const;

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
