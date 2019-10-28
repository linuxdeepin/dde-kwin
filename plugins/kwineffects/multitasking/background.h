/*
 * Copyright (C) 2019 Deepin Technology Co., Ltd.
 *
 * Author:     Sian Cao <yinshuiboy@gmail.com>
 *
 * Maintainer: Sian Cao <yinshuiboy@gmail.com>
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


#ifndef _DEEPIN_BACKGROUND_H
#define _DEEPIN_BACKGROUND_H 

#include <QObject>
#include <QPixmap>

class BackgroundManager: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString defaultNewDesktopURI READ getDefaultBackgroundURI NOTIFY defaultBackgroundURIChanged)
public:
    static BackgroundManager& instance();

    // workspace id start from 1
    QPixmap getBackground(int workspace, int monitor = 0);

    Q_INVOKABLE void shuffleDefaultBackgroundURI();
    Q_INVOKABLE QString getDefaultBackgroundURI();

signals:
    void defaultBackgroundURIChanged();

private:
    QStringList m_preinstalledWallpapers;
    QString m_defaultNewDesktopURI;

    explicit BackgroundManager();

private slots:
    void onGsettingsDDEAppearanceChanged(const QString &key);
    void onGsettingsDDEZoneChanged(const QString &key);
};


#endif /* ifndef _DEEPIN_MULTITASKING_H */



