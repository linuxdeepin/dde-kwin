// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scissorwindow.h"

class ScissorWindowPluginFactory : public KWin::EffectPluginFactory
{
    Q_OBJECT
    Q_INTERFACES(KPluginFactory)
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "scissor-window.json")

public:
    explicit ScissorWindowPluginFactory();
    ~ScissorWindowPluginFactory();

    KWin::Effect *createEffect() const {
        return ScissorWindow::supported() ? new ScissorWindow() : nullptr;
    }
};

K_PLUGIN_FACTORY_DEFINITION(ScissorWindowPluginFactory, registerPlugin<ScissorWindow>();)
K_EXPORT_PLUGIN_VERSION(KWIN_EFFECT_API_VERSION)

#include "main.moc"
