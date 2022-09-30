// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "accessibledefine.h"

#include "plugins/kwineffects/multitasking/multitasking.h"

SET_FORM_ACCESSIBLE(DesktopThumbnailManager, "desktopThumbnailManager")


QAccessibleInterface *accessibleFactory(const QString &classname, QObject *object)
{
    QAccessibleInterface *interface = nullptr;

    USE_ACCESSIBLE(classname, DesktopThumbnailManager);

    return interface;
}



