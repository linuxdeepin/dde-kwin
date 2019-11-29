import QtQuick 2.0
import QtQuick.Window 2.0
import com.deepin.kwin 1.0
import QtGraphicalEffects 1.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kwin 2.0 as KWin

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
    signal qmlRequestAppendDesktop()
    signal qmlRequestDeleteDesktop(int id)
    signal qmlRequestMove2Desktop(variant wid, int desktop)

    Component.onCompleted: {
        initDesktops();
        animateLayouting = true
    }

    Component {
        id: desktopItem

        Rectangle {
            id: thumbRoot
            color: "transparent"

            signal windowsChanged()

            width: manager.thumbSize.width
            height: manager.thumbSize.height
            property int desktop: componentDesktop

            radius: manager.currentDesktop == desktop ? 8 : 6
            //inactive border: solid 1px rgba(0, 0, 0, 0.1);
            //active border: solid 3px rgba(36, 171, 255, 1.0);
            border.width: manager.currentDesktop == desktop ? 3 : 1
            border.color: manager.currentDesktop == desktop ? Qt.rgba(0.14, 0.67, 1.0, 1.0) : Qt.rgba(0, 0, 0, 0.1)
            onWindowsChanged: {
                console.log('------------ windowsChanged')
                thumb.refreshWindows()
            }

            Drag.keys: ["wsThumb"]

            MouseArea {
                anchors.fill: parent
                drag.target: parent
                hoverEnabled: true

                onClicked: {
                    if (close.enabled) {
                        console.log("----------- change to desktop " + thumb.desktop)
                        qmlRequestChangeDesktop(thumb.desktop)
                    }
                }

                onReleased: {
                }

                onEntered: {
                    close.opacity = 1.0
                    close.enabled = true
                }

                onExited: {
                    close.opacity = 0.0
                    close.enabled = false
                }
            }
            
            DesktopThumbnail {
                id: thumb
                desktop: thumbRoot.desktop

                anchors.fill: parent
                anchors.margins: manager.currentDesktop == desktop ? 3 : 1
                radius: manager.currentDesktop == desktop ? 8 : 6

                GridView {
                    id: view
                    anchors.fill: parent
                    anchors.margins: 10
                    model: thumb.windows.length

                    interactive: false

                    cellWidth: 150
                    cellHeight: 150

                    displaced: Transition {
                        NumberAnimation { properties: "x,y"; easing.type: Easing.InOutCubic }
                    }

                    delegate: Rectangle {
                        id: viewItem

                        width: view.cellWidth
                        height: view.cellHeight
                        anchors.margins: 2
                        color: 'transparent'

                        property variant wid: thumb.windows[index]
                        property DesktopThumbnail owningDesktop: thumb

                        PlasmaCore.WindowThumbnail {
                            anchors.fill: parent
                            winId: viewItem.wid
                        }
                        //KWin.ThumbnailItem {
                            //anchors.fill: parent
                            //wId: viewItem.wid
                        //}

                        Drag.active: itemArea.drag.active
                        Drag.keys: ['winthumb']
                        Drag.hotSpot.x: width/2
                        Drag.hotSpot.y: height/2

                        states: State {
                            when: itemArea.drag.active
                            ParentChange {
                                target: viewItem
                                parent: root
                            }
                        }

                        MouseArea {
                            id: itemArea

                            anchors.fill: parent
                            drag.target: parent

                            onClicked: {
                                console.log('--------- DesktopThumbnail.window clicked ' + viewItem.wid)
                            }

                            onReleased: {
                                console.log('--------- DesktopThumbnail.window released ' + viewItem.wid)
                                if (viewItem.Drag.target != null) {
                                    // target must be a DesktopThumbnail
                                    viewItem.Drag.drop()
                                }
                                
                            }
                        }
                    }
                }
            } // ~DesktopThumbnail

            // this can accept winthumb or wsThumb type of dropping
            // winthumb is for moving window around desktops
            // wsThumb is for switching desktop positions
            DropArea {
                id: wsDrop
                anchors.fill: parent
                keys: ['winthumb', 'wsThumb']

                states: State {
                    when: wsDrop.containsDrag
                    PropertyChanges {
                        target: thumbRoot
                        border.color: "red"
                    }
                }

                onDropped: {
                    if (drop.keys[0] == 'winthumb') {
                        console.log('~~~~~ Drop winthumb, wid ' + drop.source.wid + ', to desktop ' + desktop
                            + ', from ' + drop.source.owningDesktop.desktop)

                        if (desktop != drop.source.owningDesktop.desktop)
                            qmlRequestMove2Desktop(drop.source.wid, desktop)
                    } else if (drop.keys[0] == 'wsThumb') {
                        console.log('~~~~~ Drop DesktopThumbnail, todo')
                    }
                }

                onEntered: {
                    // source could be DesktopThumbnail or winthumb
                    console.log('~~~~~  Enter ws ' + desktop + ', wid ' + drag.source.wid
                        + ', keys: ' + drag.keys)

                }
            }

            Rectangle {
                id: close
                z: 3 // at the top
                width: closeImg.width
                height: closeImg.height
                x: parent.width - closeImg.width/2
                y: -height/2
                color: "transparent"
                opacity: 0.0

                Image {
                    id: closeImg
                    source: "qrc:///icons/data/close_normal.svg"
                    sourceSize.width: 48
                    sourceSize.height: 48
                }

                Behavior on opacity {
                    PropertyAnimation { duration: 300; easing.type: Easing.InOutCubic }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        console.log("----------- close desktop " + thumb.desktop)
                        qmlRequestDeleteDesktop(thumb.desktop)
                    }

                    onEntered: {
                        closeImg.source = "qrc:///icons/data/close_hover.svg"
                    }

                    onPressed: {
                        closeImg.source = "qrc:///icons/data/close_press.svg"
                    }

                    onExited: {
                        closeImg.source = "qrc:///icons/data/close_normal.svg"
                    }
                }
            }


            Behavior on x {
                enabled: animateLayouting
                PropertyAnimation { duration: 300; easing.type: Easing.Linear }
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
        color: "#33ffffff"

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
                var plus_size = 45.0
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

        Behavior on x {
            enabled: animateLayouting
            PropertyAnimation { duration: 300; easing.type: Easing.Linear }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                qmlRequestAppendDesktop()
            }
            onEntered: {
                backgroundManager.shuffleDefaultBackgroundURI()
                background.opacity = 0.6
            }

            onExited: {
                background.opacity = 0.0
            }
        }
    }

    function handleAppendDesktop() {
        var id = manager.desktopCount
        console.log('--------------- handleAppendDesktop ' + manager.desktopCount)

        var src = 'import QtQuick 2.0; Loader { sourceComponent: desktopItem; ' + 
        'property int componentDesktop: ' + id + '}';
        var obj = Qt.createQmlObject(src, root, "dynamicSnippet"); 
        var r = manager.calculateDesktopThumbRect(id-1);
        obj.x = r.x
        obj.y = r.y
        thumbs.append({'obj': obj});
    }

    function handleDesktopRemoved(id) {
        console.log('--------------- handleDesktopRemoved ' + id)
        for (var i = 0; i < thumbs.count; i++) {
            var d = thumbs.get(i)
            if (d.obj.componentDesktop == id) {
                d.obj.destroy()
                thumbs.remove(i)
                break;
            }
        }
    }

    function handleDesktopWindowsChanged(id) {
        for (var i = 0; i < thumbs.count; i++) {
            var d = thumbs.get(i)
            if (d.obj.componentDesktop == id) {
                console.log('------------- handleDesktopWindowsChanged: ' + id)
                d.obj.item.windowsChanged();
                break;
            }
        }
    }


    function debugObject(o) {
        //for (var p in Object.getOwnPropertyNames(o)) {
            //console.log("========= " + o[p]);
        //}

        var keys = Object.keys(o);
        for(var i=0; i<keys.length; i++) {
            var key = keys[i];
            // prints all properties, signals, functions from object
            console.log('======== ' + key + ' : ' + o[key]);
        }
    }

    function handleLayoutChanged() {
        console.log('--------------- layoutChanged')
        var r = manager.calculateDesktopThumbRect(manager.desktopCount);
        plus.x = r.x
        plus.y = r.y

        if (manager.desktopCount < thumbs.count) {
            // this has been handled by handleDesktopRemoved
        }

        for (var i = 0; i < thumbs.count; i++) {
            var r = manager.calculateDesktopThumbRect(i);
            thumbs.get(i).obj.x = r.x
            thumbs.get(i).obj.y = r.y
            thumbs.get(i).obj.componentDesktop = i+1
        }

        // rearrange thumbs
        if (manager.desktopCount > thumbs.count) {
            handleAppendDesktop();
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

}

