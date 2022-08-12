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
#include "scissorwindow.h"

class ScissorWindowPluginFactory : public KWin::EffectPluginFactory
{
    Q_OBJECT
#if KWIN_VERSION_MAJ <= 5 && KWIN_VERSION_MIN < 23
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "scissor-window.json")
#else
    Q_PLUGIN_METADATA(IID EffectPluginFactory_iid FILE "scissor-window.json")
#endif
    Q_INTERFACES(KPluginFactory)
public:
    explicit ScissorWindowPluginFactory() {}
    ~ScissorWindowPluginFactory() {}

    bool isSupported() const override {
        return ScissorWindow::supported();
    }

    bool enabledByDefault() const override {
        return true;
    }

    KWin::Effect *createEffect() const override {
        return new ScissorWindow;
    }
};

#include "main.moc"
