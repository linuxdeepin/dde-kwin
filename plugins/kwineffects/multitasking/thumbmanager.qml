import QtQuick 2.0
import QtQuick.Window 2.0
import com.deepin.kwin 1.0
import QtGraphicalEffects 1.0

Rectangle {
    id: root

    color: "transparent"

    x: 0
    y: 0
    width: manager.containerSize.width
    height: manager.containerSize.height

    property int currentDesktop: manager.currentDesktop
    property bool animateLayouting: false

    signal qmlRequestChangeDesktop(int to)

    Component.onCompleted: {
        initDesktops();
        animateLayouting = true
    }

    Component {
        id: desktopItem

        Rectangle {
            color: "transparent"

            width: manager.thumbSize.width
            height: manager.thumbSize.height

            radius: manager.currentDesktop == thumb.desktop ? 8 : 6
            //inactive border: solid 1px rgba(0, 0, 0, 0.1);
            //active border: solid 3px rgba(36, 171, 255, 1.0);
            border.width: manager.currentDesktop == thumb.desktop ? 3 : 1
            border.color: manager.currentDesktop == thumb.desktop ? Qt.rgba(0.14, 0.67, 1.0, 1.0) : Qt.rgba(0, 0, 0, 0.1)
            

            DesktopThumbnail {
                id: thumb
                desktop: componentDesktop

                anchors.fill: parent
                anchors.margins: manager.currentDesktop == desktop ? 3 : 1
                radius: manager.currentDesktop == desktop ? 8 : 6
            }

            Behavior on x {
                enabled: animateLayouting
                PropertyAnimation { duration: 300; easing.type: Easing.Linear }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    qmlRequestChangeDesktop(thumb.desktop)
                }
            }
        }

    }

    ListModel {
        id: thumbs
    }

    Rectangle {
        id: plus
        visible: manager.showPlusButton
        enabled: visible

        x: 0
        y: 0
        width: manager.thumbSize.width
        height: manager.thumbSize.height
        radius: 6

        Image {
            z: 1
            id: background
            source: backgroundManager.defaultNewDesktopURI
            anchors.fill: parent

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
                var plus_size = 45.0
                ctx.lineWidth = 2
                ctx.strokeStyle = "rgba(255, 255, 255, 0.8)";

                ctx.beginPath();
                ctx.moveTo((width - plus_size)/2, height/2);
                ctx.lineTo((width + plus_size)/2, height/2);

                ctx.moveTo(width/2, (height - plus_size)/2);
                ctx.lineTo(width/2, (height + plus_size)/2);
                ctx.stroke();
            }
        }

        Behavior on x {
            enabled: animateLayouting
            PropertyAnimation { duration: 300; easing.type: Easing.Linear }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                console.log('---------------- click PLUS')
            }
            onEntered: {
                console.log('---------------- enter PLUS')
                backgroundManager.shuffleDefaultBackgroundURI()
            }
        }
    }

    function handleAppendDesktop() {
        console.log('--------------- handleAppendDesktop')
    }

    function handleDesktopRemoved() {
        console.log('--------------- handleDesktopRemoved')
    }

    function handleLayoutChanged() {
        console.log('--------------- layoutChanged')
        var r = manager.calculateDesktopThumbRect(manager.desktopCount);
        plus.x = r.x
        plus.y = r.y

        // rearrange thumbs
        if (manager.desktopCount > thumbs.count) {
            handleAppendDesktop();
        } else if (manager.desktopCount < thumbs.count) {
            handleDesktopRemoved();
        }

    }

    function initDesktops() {
        var r = manager.calculateDesktopThumbRect(manager.desktopCount);
        plus.x = r.x
        plus.y = r.y

        for (var i = 1; i <= manager.desktopCount; i++) {
            var src = 'import QtQuick 2.0; Loader { sourceComponent: desktopItem; ' + 
            'property int componentDesktop: ' + i + '}';
            var obj = Qt.createQmlObject(src, root, "dynamicSnippet"); 
            var r = manager.calculateDesktopThumbRect(i-1);
            obj.x = r.x
            obj.y = r.y
            thumbs.append({'obj': obj});
        }
    }

    function handleDesktopCountChanged() {
        console.log('--------------- desktopCountChanged')
    }

}

