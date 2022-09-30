// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "chameleon.h"
#include "chameleonconfig.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(
    ChameleonDecoFactory,
    "chameleon.json",
    registerPlugin<Chameleon>();
)

__attribute__((constructor))
static void _init_theme()
{
    // make sure atoms are initialized during the window manager startup stage
    auto global_config = ChameleonConfig::instance();
}

#include "main.moc"
