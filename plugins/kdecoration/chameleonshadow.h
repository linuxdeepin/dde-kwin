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
#ifndef CHAMELEONSHADOW_H
#define CHAMELEONSHADOW_H

#include "chameleon.h"

#include <KDecoration2/DecorationShadow>

#include <QSharedPointer>

class ChameleonShadow
{
public:
    static ChameleonShadow *instance();

    QSharedPointer<KDecoration2::DecorationShadow> getShadow(const Chameleon *client);

protected:
    ChameleonShadow();

private:
    QMap<QString, QSharedPointer<KDecoration2::DecorationShadow>> m_shadowCache;
};

#endif // CHAMELEONSHADOW_H
