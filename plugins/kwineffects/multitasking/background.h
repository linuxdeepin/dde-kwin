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

#include "wm_interface.h"

class BackgroundManager: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString defaultNewDesktopURI READ getDefaultBackgroundURI NOTIFY defaultBackgroundURIChanged)
public:
    static BackgroundManager& instance();

    void updateDesktopCount(int n) {
        if (m_desktopCount != n) {
            m_desktopCount = n;
        }
    }

    // workspace id start from 1
    QPixmap getBackground(int workspace, QString screenName, const QSize& size = QSize());
    QPixmap getBackgroundPixmap(int workSpace, QString screenName);
    Q_INVOKABLE void shuffleDefaultBackgroundURI();
    Q_INVOKABLE QString getDefaultBackgroundURI();

    void changeWorkSpaceBackground(int workspaceIndex);

    void setMonitorInfo(QList<QMap<QString,QVariant>> monitorInfoLst);
public slots:
    // respond to desktop removal, and shift wallpapers accordingly
    void desktopAboutToRemoved(int d);
    void desktopSwitchedPosition(int to, int from);

signals:
    void defaultBackgroundURIChanged();
    void wallpapersChanged();
    void desktopWallpaperChanged(int d);

private:
    QStringList m_preinstalledWallpapers;
    QString m_defaultNewDesktopURI;
    int m_desktopCount {0};
    QStringList m_cachedUris;
    int m_monitorIndex {0};
    QList<QString> m_screenNamelst;

    QHash<QString, QPair<QSize, QPixmap>> m_cachedPixmaps;

    QHash<QString, QPair<QSize, QPixmap>> m_bigCachedPixmaps;

    QList<QMap<QString,QVariant>> m_monitorInfoLst;
    QScopedPointer<ComDeepinWmInterface> m_wm_interface;

    explicit BackgroundManager();

private slots:
    void onGsettingsDDEAppearanceChanged(const QString &key);
};


#endif /* ifndef _DEEPIN_MULTITASKING_H */



