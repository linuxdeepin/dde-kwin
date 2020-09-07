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
#ifndef SCISSORWINDOW_H
#define SCISSORWINDOW_H

#include <kwineffects.h>

class ScissorWindow : public KWin::Effect
{
    Q_OBJECT
public:
    enum DataRole {
        BaseRole = KWin::DataRole::LanczosCacheRole + 100,
        WindowRadiusRole = BaseRole + 1,
        WindowClipPathRole = BaseRole + 2,
        WindowMaskTextureRole = BaseRole + 3,
        WindowDepthRole = BaseRole + 4
    };

    static bool supported();

    explicit ScissorWindow(QObject *parent = nullptr, const QVariantList &args = QVariantList());

#if KWIN_VERSION_MIN > 17 || (KWIN_VERSION_MIN == 17 && KWIN_VERSION_PAT > 5)
    void drawWindow(KWin::EffectWindow* w, int mask, const QRegion &region, KWin::WindowPaintData& data) override;
#else
    void drawWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data) override;
#endif

private:
    KWin::GLShader *m_shader = nullptr;
    KWin::GLShader *m_fullMaskShader = nullptr;
};

#endif // SCISSORWINDOW_H
