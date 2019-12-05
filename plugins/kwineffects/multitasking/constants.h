/*
 * Copyright (C) 2019 Deepin Technology Co., Ltd.
 *
 * Author:     Sian Cao <yinshuiboy@gmail.com>
 *
 * Maintainer: Sian Cao <yinshuiboy@gmail.com>
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


#ifndef _DEEPIN_CONSTANTS_H
#define _DEEPIN_CONSTANTS_H 

#include <QEasingCurve>

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
}

#endif
