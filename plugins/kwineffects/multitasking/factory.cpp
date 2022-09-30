// Copyright (C) 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../../accessible.h"
#include "multitasking.h"

//KWIN_EFFECT_FACTORY(MultitaskingPluginFactory, MultitaskingEffect, "multitasking.json")

class MultitaskingPluginFactory : public KWin::EffectPluginFactory
{
    Q_OBJECT
    Q_INTERFACES(KPluginFactory)
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "multitasking.json")

public:
    explicit MultitaskingPluginFactory() {}
    ~MultitaskingPluginFactory() {}

    KWin::Effect *createEffect() const override {
        KWin::Effect *e = new  MultitaskingEffect();
        QAccessible::installFactory(accessibleFactory);
        return e;
    }
};

K_EXPORT_PLUGIN_VERSION(KWIN_EFFECT_API_VERSION)

#include "factory.moc"
