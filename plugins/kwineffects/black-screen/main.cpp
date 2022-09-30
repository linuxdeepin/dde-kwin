// Copyright (C) 2020 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
