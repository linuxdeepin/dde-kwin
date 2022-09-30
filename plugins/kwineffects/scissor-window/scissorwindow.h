// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
