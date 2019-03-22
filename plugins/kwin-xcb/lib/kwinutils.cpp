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
#include "kwinutils.h"

#include <QLibrary>
#include <QCoreApplication>
#include <QDebug>

class KWinInterface
{
    typedef int (*ClientMaximizeMode)(void *);
    typedef void (*ClientMaximize)(void *, KWinUtils::MaximizeMode);
public:
    KWinInterface()
    {
        clientMaximizeMode = (ClientMaximizeMode)KWinUtils::resolve("_ZNK4KWin6Client12maximizeModeEv");
        clientMaximize = (ClientMaximize)KWinUtils::resolve("_ZN4KWin14AbstractClient8maximizeENS_12MaximizeModeE");
    }

    ClientMaximizeMode clientMaximizeMode;
    ClientMaximize clientMaximize;
};

Q_GLOBAL_STATIC(KWinInterface, interface)

KWinUtils::KWinUtils(QObject *parent)
    : QObject(parent)
{

}

KWinUtils::~KWinUtils()
{

}

QFunctionPointer KWinUtils::resolve(const char *symbol)
{
    static QString lib_name = "kwin.so." + qApp->applicationVersion();

    return QLibrary::resolve(lib_name, symbol);
}

QVariant KWinUtils::isFullMaximized(QObject *window) const
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximizeMode) {
        return QVariant();
    }

    return interface->clientMaximizeMode(window) == MaximizeFull;
}

QVariant KWinUtils::fullmaximizeWindow(QObject *window) const
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximize) {
        return QVariant();
    }

    interface->clientMaximize(window, MaximizeFull);

    return true;
}

QVariant KWinUtils::unaximizeWindow(QObject *window) const
{
    if (!window) {
        return false;
    }

    if (!interface->clientMaximize) {
        return QVariant();
    }

    interface->clientMaximize(window, MaximizeRestore);

    return true;
}
