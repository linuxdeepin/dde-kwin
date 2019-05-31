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

#include "chameleon.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(
    BreezeDecoFactory,
    "chameleon.json",
    registerPlugin<Chameleon>();
)

extern "C" {
    Q_DECL_EXPORT QString kde_plugin_version()
    {
        return PROJECT_VERSION;
    }
}

#include "main.moc"
