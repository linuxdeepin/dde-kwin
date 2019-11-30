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

            //FIXME: define a enum {
            //    PendingRemove,
            //    PendingSwitch
            //}
            property bool pendingDragRemove: false

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
            Drag.active: thumbArea.drag.active
            //TOOD: should be cursor position?
            Drag.hotSpot {
                x: width/2
                y: height/2
            }

            states: State {
                when: thumbArea.drag.active
                ParentChange {
                    target: thumbRoot
                    parent: root
                }

                PropertyChanges {
                    target: thumbRoot
                    z: 100
                }
            }

            MouseArea {
                id: thumbArea
                anchors.fill: parent
                drag.target: parent
                hoverEnabled: true

                onClicked: {
                    if (close.enabled) {
                        console.log("----------- change to desktop " + thumb.desktop)
                        qmlRequestChangeDesktop(thumb.desktop)
                    }
                }

                onPressed: {
                    //FIXME: make hotSpot follow mouse cursor when drag started
                    // however, this is not accurate, we need a drag-started event
                    thumbRoot.Drag.hotSpot.x = mouse.x
                    thumbRoot.Drag.hotSpot.y = mouse.y
                }

                onReleased: {
                    // target should be wsDropComponent
                    if (thumbRoot.Drag.target != null) {
                        console.log('------- release ws on ' + thumbRoot.Drag.target)
                        thumbRoot.Drag.drop()
                    } else {
                        //FIXME: not good, since current the parent is still chagned (by ParentChange)
                        thumbRoot.x = 0
                        thumbRoot.y = 0
                    }
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

            // this can accept winthumb type of dropping
            // winthumb is for moving window around desktops
            DropArea {
                id: winDrop
                anchors.fill: parent
                keys: ['winthumb']

                states: State {
                    when: winDrop.containsDrag
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
                    }
                }

                onEntered: {
                    // source could be DesktopThumbnail or winthumb
                    if (drag.keys[0] == 'winthumb') {
                        console.log('~~~~~  Enter ws ' + desktop + ', wid ' + drag.source.wid
                        + ', keys: ' + drag.keys)
                    } else {
                        drag.accepted = false
                    }

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

                            PropertyChanges {
                                target: viewItem
                                z: 100
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

            Behavior on y {
                enabled: animateLayouting
                PropertyAnimation { duration: 300; easing.type: Easing.Linear }
            }

        }
    }

    Component {
        id: wsDropComponent

        DropArea {
            id: wsDrop
            width: manager.thumbSize.width
            height: manager.thumbSize.height
            property int designated: index

            z: 1
            keys: ['wsThumb']

            onDropped: {
                /* NOTE:
                 * during dropping, PropertyChanges is still in effect, which means 
                 * drop.source.parent should not be Loader
                 * and drop.source.z == 100
                 */
                if (drop.keys[0] === 'wsThumb') {
                    console.log('------wsDrop on ws ' + wsDrop.designated + ', switch with ' + drop.source.desktop)
                    if (wsDrop.designated == drop.source.desktop && drop.source.pendingDragRemove) {
                        //FIXME: could be a delete operation but need more calculation
                        console.log("----------- close desktop " + drop.source.desktop)
                        qmlRequestDeleteDesktop(drop.source.desktop)
                    } else {
                        drop.source.x = 0
                        drop.source.y = 0
                    }
                }
            }

            onEntered: {
                if (drag.keys[0] === 'wsThumb') {
                    console.log('------[wsDrop]: Enter ' + wsDrop.designated + ' from ' + drag.source
                        + ', keys: ' + drag.keys + ', accept: ' + drag.accepted)
                }
            }

            onPositionChanged: {
                if (drag.keys[0] === 'wsThumb') {
                    console.log('------ ' + drag.x + ',' + drag.y)
                    if ( drag.y < 30) {
                        hint.visible = true
                    } else {
                        hint.visible = false
                    }
                    drag.source.pendingDragRemove = hint.visible
                }
            }

            Rectangle {
                id: hint
                visible: false
                anchors.fill: parent
                color: 'lightblue'

                Text {
                    text: "Drag upwards to remove"
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: parent.height * 0.572

                    font.family: "Helvetica"
                    font.pointSize: 14
                    color: Qt.rgba(1, 1, 1, 0.5)
                }

                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.lineWidth = 0.5;
                        ctx.strokeStyle = "rgba(255, 255, 255, 0.6)";

                        var POSITION_PERCENT = 0.449;
                        var LINE_START = 0.060;

                        ctx.beginPath();
                        ctx.moveTo(width * LINE_START, height * POSITION_PERCENT);
                        ctx.lineTo(width * (1.0 - 2.0 * LINE_START), height * POSITION_PERCENT);
                        ctx.stroke();
                    }
                }
            }
        }
    }


    // list of wwsDropComponent
    ListModel {
        id: placeHolds
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
    } //~ plus button

    function newDesktop(desktop) {
        var r = manager.calculateDesktopThumbRect(desktop-1);

        var src = 'import QtQuick 2.0; Loader { sourceComponent: desktopItem; ' + 
        'property int componentDesktop: ' + desktop + '}';
        var obj = Qt.createQmlObject(src, root, "dynamicSnippet"); 
        obj.x = r.x
        obj.y = r.y
        obj.z = 2
        thumbs.append({'obj': obj});

        var src2 = 'import QtQuick 2.0; Loader { sourceComponent: wsDropComponent; ' + 
        'property int index: ' + desktop + '}';
        var obj2 = Qt.createQmlObject(src2, root, "dynamicSnippet2"); 
        obj2.x = r.x
        obj2.y = r.y
        obj2.z = 1
        placeHolds.append({'obj': obj2});
    }

    function handleAppendDesktop() {
        var id = manager.desktopCount
        console.log('--------------- handleAppendDesktop ' + manager.desktopCount)

        newDesktop(id)
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

        for (var i = 0; i < placeHolds.count; i++) {
            var d = placeHolds.get(i)
            if (d.obj.index == id) {
                d.obj.destroy()
                placeHolds.remove(i)
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

            placeHolds.get(i).obj.x = r.x
            placeHolds.get(i).obj.y = r.y
            placeHolds.get(i).obj.index = i+1
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
            newDesktop(i)
        }
    }

}

