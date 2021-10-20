/*
 * Copyright (C) 2020 ~ 2022 Deepin Technology Co., Ltd.
 *
 * Author:     tanfang <tanfang@uniontech.com>
 *
 * Maintainer: tanfang <tanfang@uniontech.com>
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

#include "black-screen.h"

class BlackScreenPluginFactory : public KWin::EffectPluginFactory
{
    Q_OBJECT
    Q_INTERFACES(KPluginFactory)
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "black-screen.json")

public:
    explicit BlackScreenPluginFactory();
    ~BlackScreenPluginFactory();

    KWin::Effect *createEffect() const {
        return BlackScreenEffect::supported() ? new BlackScreenEffect() : nullptr;
    }
};

K_PLUGIN_FACTORY_DEFINITION(BlackScreenPluginFactory, registerPlugin<BlackScreenEffect>();)
K_EXPORT_PLUGIN_VERSION(KWIN_EFFECT_API_VERSION)

#include "main.moc"
