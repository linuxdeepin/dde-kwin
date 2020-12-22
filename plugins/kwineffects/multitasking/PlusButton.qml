/*
 * Copyright (C) 2020 Uniontech Technology Co., Ltd.
 *
 * Author:     yunqiang tai <taiyunqiang@uniontech.com>
 *
 * Maintainer: yunqiang tai <taiyunqiang@uniontech.com>
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

import QtQuick 2.0
import QtGraphicalEffects 1.0

Rectangle {
    id: plus
    enabled: visible
    color: "#33ffffff"

    radius: width > 120 ? 30: 15

    Image {
        z: 1
        id: background
        anchors.fill: parent
        visible: false //disable now

        opacity: 0.0
        Behavior on opacity {
            PropertyAnimation { duration: 200; easing.type: Easing.InOutCubic }
        }

        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Rectangle {
                x: background.x
                y: background.y
                width: background.width
                height: background.height
                radius: 6
            }
        }
    }

    Canvas {
        z: 2
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            var plus_size = 20.0
            ctx.lineWidth = 2
            ctx.strokeStyle = "rgba(255, 255, 255, 1.0)";

            ctx.beginPath();
            ctx.moveTo((width - plus_size)/2, height/2);
            ctx.lineTo((width + plus_size)/2, height/2);

            ctx.moveTo(width/2, (height - plus_size)/2);
            ctx.lineTo(width/2, (height + plus_size)/2);
            ctx.stroke();
        }
    }

    MouseArea {
        anchors.fill: parent

        Accessible.role: Accessible.Button
        Accessible.name: "Ma_plusBtn"
        Accessible.description: "plus Button"
        Accessible.onPressAction: pressed()

        onClicked: {
            multitaskingModel.append();
            multitaskingModel.setCurrentIndex(multitaskingModel.rowCount() - 1);
        }
        onEntered: {
            background.opacity = 0.6
        }

        onExited: {
            background.opacity = 0.0
        }
    }
} //~ plus button
