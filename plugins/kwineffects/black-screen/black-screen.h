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

#ifndef BLACKSCREENEFFECT_H
#define BLACKSCREENEFFECT_H

#include <kwineffects.h>
#include <kwinglplatform.h>
#include <kwinglutils.h>
#include <QDBusContext>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>

using namespace KWin;

class BlackScreenEffect : public KWin::Effect, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kwin.BlackScreen")
public:
    static bool supported();

    BlackScreenEffect(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    ~BlackScreenEffect() override;

    void drawWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data) override;
    virtual bool isActive() const override;
    int requestedEffectChainPosition() const override {
        return 1;
    }
public Q_SLOTS:
    Q_SCRIPTABLE void setActive(bool active);
    Q_SCRIPTABLE bool getActive();
private:
    bool m_activated {false};
};

#endif // BLACKSCREENEFFECT_H
