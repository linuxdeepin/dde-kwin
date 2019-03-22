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
#ifndef KWINUTILS_H
#define KWINUTILS_H

#include <QObject>
#include <QVariant>

class KWinUtils : public QObject
{
    Q_OBJECT
public:
    enum MaximizeMode {
        MaximizeRestore    = 0, ///< The window is not maximized in any direction.
        MaximizeVertical   = 1, ///< The window is maximized vertically.
        MaximizeHorizontal = 2, ///< The window is maximized horizontally.
        /// Equal to @p MaximizeVertical | @p MaximizeHorizontal
        MaximizeFull = MaximizeVertical | MaximizeHorizontal
    };

    explicit KWinUtils(QObject *parent = nullptr);
    ~KWinUtils();

    static QFunctionPointer resolve(const char *symbol);

    Q_INVOKABLE QVariant isFullMaximized(QObject *window) const;
    Q_INVOKABLE QVariant fullmaximizeWindow(QObject *window) const;
    Q_INVOKABLE QVariant unaximizeWindow(QObject *window) const;
};

#endif // KWINUTILS_H
