// Copyright (C) 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _DEEPIN_CONSTANTS_H
#define _DEEPIN_CONSTANTS_H 

#include <QEasingCurve>
#include <QtCore>

namespace Constants {
    const QEasingCurve TOGGLE_MODE =  QEasingCurve::OutQuint;// AnimationMode.EASE_OUT_QUINT;
    static const int WORKSPACE_SWITCH_DURATION = 400;
    static const int WORKSPACE_FADE_DURATION = 500;

    static const int SMOOTH_SCROLL_DELAY = 500;

    /**
     * The percent value between workspace clones' horizontal offset and monitor's height.
     */
    static const float HORIZONTAL_OFFSET_PERCENT = 0.044f;

    /**
     * The percent value between flow workspace's top offset and monitor's height.
     */
    static const float FLOW_WORKSPACE_TOP_OFFSET_PERCENT = 0.161f;

    /**
     * The percent value between distance of flow workspaces and its width.
     */
    static const float FLOW_WORKSPACE_DISTANCE_PERCENT = 0.089f;

    static const float WORKSPACE_WIDTH_PERCENT = 0.12f;
        
    /**
     * The percent value between distance of thumbnail workspace clones and monitor's width.
     */
    static const float SPACING_PERCENT = 0.02f;

    /**
     * thumbnail workspace relayout duration 
     */
    static const int RELAYOUT_DURATION = 300;

    /**
     * size for window icon
     */
    static const int ICON_SIZE = 64;
    /**
     * size for window action icons: pin, unpin, close
     */
    static const int ACTION_SIZE = 48;

    static const float HIGHLIGHT_SCALE = 1.05f;
}

Q_DECLARE_LOGGING_CATEGORY(BLUR_CAT)


#endif
