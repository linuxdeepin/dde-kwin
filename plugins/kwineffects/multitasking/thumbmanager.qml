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
    signal qmlRequestSwitchDesktop(int to, int from)

    signal mouseLeaved(); // mouse leaved thumbmanager

    function log(msg) {
        manager.debugLog(msg)
    }

    onVisibleChanged: {
        log(' !!!------- thumbmanager visible ' + visible)
    }

    Component.onCompleted: {
        initDesktops();
        animateLayouting = true
    }

    onMouseLeaved: {
        log(' !!!------- leaved thumbmanager')
    }

    Component {
        id: desktopItem

        Rectangle {
            id: thumbRoot
            color: "transparent"

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

            Drag.keys: ["wsThumb"]
            //NOTE: need to bind to thumbArea.pressed 
            //when mouse pressed and leave DesktopThumbnailManager, drag keeps active 
            //while mouse released or not later.
            Drag.active: manager.desktopCount > 1 && thumbArea.pressed && thumbArea.drag.active
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

            // make sure dragged ws reset back to Loader's (0, 0)
            Timer {
                id: timerBack
                repeat: false
                interval: 1
                running: false
                onTriggered: {
                    log('~~~~~~ restore position')
                    parent.x = 0
                    parent.y = 0

                    parent.lastDragX = -1
                    parent.lastDragY = 0
                }
            }

            /* 
             * this is a hack to make a smooth bounce back animation
             * thumbRoot'll be reparnet back to loader and make a sudden visual
             * change of thumbRoot's position. we can disable behavior animation 
             * and set position to the same visual point in the scene (where mouse
             * resides), and then issue the behavior animation.
             */
            property int lastDragX: -1
            property int lastDragY: 0
            property bool disableBehavior: false
            onParentChanged: {
                if (parent != root && lastDragX != -1) {
                    log('~~~~~~~ parent chagned to ' + parent)
                    var pos = parent.mapFromGlobal(lastDragX, lastDragY)
                    log('----- ' + parent.x + ',' + parent.y + " => " + pos.x + ',' + pos.y)
                    thumbRoot.x = pos.x
                    thumbRoot.y = pos.y
                    if (pendingDragRemove) {
                        disableBehavior = false
                        timerBack.running = false
                    }
                }
            }

            MouseArea {
                id: thumbArea
                anchors.fill: parent
                drag.target: null
                hoverEnabled: true

                onClicked: {
                    if (close.enabled) {
                        log("----------- change to desktop " + thumb.desktop)
                        qmlRequestChangeDesktop(thumb.desktop)
                    }
                }

                onPositionChanged: {
                    // this could happen when mouse press-hold and leave DesktopThumbnailManager
                    if (!thumbArea.pressed && drag.target != null) {
                        drag.target = null
                    }
                }

                onPressed: {
                    //FIXME: make hotSpot follow mouse cursor when drag started
                    // however, this is not accurate, we need a drag-started event
                    thumbRoot.Drag.hotSpot.x = mouse.x
                    thumbRoot.Drag.hotSpot.y = mouse.y

                    if (manager.desktopCount > 1) {
                        drag.target = parent
                    }
                }

                onReleased: {
                    if (manager.desktopCount == 1) {
                        return
                    }

                    // target should be wsDropComponent
                    if (thumbRoot.Drag.target != null) {
                        log('------- release ws on ' + thumbRoot.Drag.target)
                        thumbRoot.Drag.drop()
                    }

                    //NOTE: since current the parent is still chagned (by ParentChange), 
                    //delay (x,y) reset into timerBack
                    timerBack.running = true
                    log('----- mouse release: ' + parent.x + ',' + parent.y)
                    parent.lastDragX = parent.x
                    parent.lastDragY = parent.y
                    parent.disableBehavior = true
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
                        border.color: Qt.rgba(0.14, 0.67, 1.0, 0.5)
                    }
                }

                onDropped: {
                    if (drop.keys[0] == 'winthumb') {
                        log('~~~~~ Drop winthumb, wid ' + drop.source.wid + ', to desktop ' + desktop
                            + ', from ' + drop.source.owningDesktop.desktop)

                        if (desktop != drop.source.owningDesktop.desktop)
                            qmlRequestMove2Desktop(drop.source.wid, desktop)
                    }
                }

                onEntered: {
                    // source could be DesktopThumbnail or winthumb
                    if (drag.keys[0] == 'winthumb') {
                        log('~~~~~  Enter ws ' + desktop + ', wid ' + drag.source.wid
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

                onWindowsLayoutChanged: {
                    for (var i = 0; i < children.length; i++) {
                        if (children[i].objectName == 'repeater')
                            continue;
                        var geo = geometryForWindow(thumb.children[i].wid)
                        thumb.children[i].x = geo.x
                        thumb.children[i].y = geo.y
                        thumb.children[i].width = geo.width
                        thumb.children[i].height = geo.height
                        log('  --- relayout ' + desktop + ' ' + geo);
                    }
                }

                Repeater {
                    id: view
                    objectName: 'repeater'

                    model: thumb.windows.length

                    property int cellWidth: 150
                    property int cellHeight: 150

                    delegate: Rectangle {
                        id: viewItem

                        width: view.cellWidth
                        height: view.cellHeight
                        clip: true
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

                        //NOTE: need to bind to itemArea.pressed
                        //when mouse pressed and leave DesktopThumbnailManager, drag keeps active
                        //while mouse released or not later.
                        Drag.active: itemArea.pressed && itemArea.drag.active
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

                        // make sure dragged ws reset back to Loader's (0, 0)
                        Timer {
                            id: timerBack
                            repeat: false
                            interval: 1
                            running: false
                            onTriggered: {
                                log('~~~~~~ restore winthumb position')

                                var geo = thumb.geometryForWindow(wid)
                                viewItem.x = geo.x
                                viewItem.y = geo.y

                                lastDragX = -1
                                lastDragY = 0
                            }
                        }

                        property int lastDragX: -1
                        property int lastDragY: 0
                        property bool disableBehavior: false
                        onParentChanged: {
                            if (parent != root && lastDragX != -1) {
                                var pos = parent.mapFromGlobal(lastDragX, lastDragY)
                                viewItem.x = pos.x
                                viewItem.y = pos.y

                                log('~~~~~~~ winthumb parent chagned to ' + parent +
                                    ' at ' + pos.x + ',' + pos.y)
                                disableBehavior = false
                            }
                        }

                        MouseArea {
                            id: itemArea

                            anchors.fill: parent
                            drag.target: null

                            onPositionChanged: {
                                // this could happen when mouse press-hold and leave DesktopThumbnailManager
                                if (!itemArea.pressed && drag.target != null) {
                                    drag.target = null
                                }
                            }

                            onPressed: {
                                drag.target = parent
                                // Click the window in the thumbnail to switch workspaces
                                qmlRequestChangeDesktop(thumb.desktop)
                            }

                            onReleased: {
                                log('--------- DesktopThumbnail.window released ' + viewItem.wid)
                                if (viewItem.Drag.target != null) {
                                    // target must be a DesktopThumbnail
                                    viewItem.Drag.drop()
                                } 
                                timerBack.running = true
                                parent.lastDragX = parent.x
                                parent.lastDragY = parent.y
                                parent.disableBehavior = true
                            }
                        }
                    } // ~ViewItem
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
                visible: (1 != thumbs.count) //The close button is not displayed when there is only one thumbnail

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
                        log("----------- close desktop " + thumb.desktop)
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

            DropShadow {
                anchors.fill: parent
                verticalOffset: 5
                spread: 0.2
                radius: 8.0
                samples: 17
                color: "#1A000000"
                source: parent
            }

            Behavior on x {
                enabled: animateLayouting && !disableBehavior
                PropertyAnimation { duration: 300; easing.type: Easing.Linear }
            }

            Behavior on y {
                enabled: animateLayouting && !disableBehavior
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
                    var from = drop.source.desktop
                    var to = wsDrop.designated
                    if (wsDrop.designated == drop.source.desktop && drop.source.pendingDragRemove) {
                        //FIXME: could be a delete operation but need more calculation
                        log("----------- wsDrop: close desktop " + from)
                        qmlRequestDeleteDesktop(from)
                    } else {
                        if (from == to) {
                            return
                        }
                        log("----------- wsDrop: reorder desktop ")

                        thumbs.move(from-1, to-1, 1)
                        qmlRequestSwitchDesktop(to, from)
                        // must update layout right now
                        handleLayoutChanged()
                    }
                }
            }

            onEntered: {
                if (drag.keys[0] === 'wsThumb') {
                    log('------[wsDrop]: Enter ' + wsDrop.designated + ' from ' + drag.source
                        + ', keys: ' + drag.keys + ', accept: ' + drag.accepted)
                }
            }

            onExited: {
                if (drag.source.pendingDragRemove) {
                    hint.visible = false
                    drag.source.pendingDragRemove = hint.visible
                }

            }

            onPositionChanged: {
                if (drag.keys[0] === 'wsThumb') {
                    var diff = wsDrop.parent.y - drag.source.y
                    //log('------ ' + wsDrop.parent.y + ',' + drag.source.y + ', ' + diff)
                    if (diff > 0 && diff > drag.source.height/2) {
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
                color: 'transparent'

                Text {
                    text: qsTr("Drag upwards to remove")
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

        x: 3000
        y: 0
        width: parent.height/2
        height: parent.height/2
        radius: width > 120 ? 30: 15

        Connections {
            target: root
            onWidthChanged: {
                var r = manager.calculateDesktopThumbRect(0)
                plus.x = manager.containerSize.width - 200
                plus.y = r.y + (r.height - plus.height)/2
                log(' ------------ width changed ' + root.width)
            }
        }
        Image {
            z: 1
            id: background
            //source: backgroundManager.defaultNewDesktopURI
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

        Behavior on x {
            enabled: animateLayouting
            PropertyAnimation { duration: 300; easing.type: Easing.Linear }
        }

        MouseArea {
            anchors.fill: parent
            //hoverEnabled: true
            onClicked: {
                qmlRequestAppendDesktop()
            }
            onEntered: {
                //backgroundManager.shuffleDefaultBackgroundURI()
                background.opacity = 0.6
            }

            onExited: {
                background.opacity = 0.0
            }
        }
    } //~ plus button

    function newDesktop(desktop) {
        var r = manager.calculateDesktopThumbRect(desktop-1);

        var src = 'import QtQuick 2.0; Loader { sourceComponent: desktopItem; \n' +
            ' property bool loading: true;' +
            ' Behavior on x { ' + 
            '   enabled: animateLayouting && !loading; ' +
            '   PropertyAnimation {' +
            '     onRunningChanged: { log("--------- running " + running); }\n' +
            '     duration: 300; easing.type: Easing.Linear } } ' +
            ' x: ' + r.x + ';' +
            ' y: ' + r.y + ';' +
            ' property int componentDesktop: ' + desktop + '}';
        var obj = Qt.createQmlObject(src, root, "dynamicSnippet"); 
        obj.z = 2
        obj.loading = false;
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
        log('--------------- handleAppendDesktop ' + manager.desktopCount)

        newDesktop(id)
    }

    function handleDesktopRemoved(id) {
        log('--------------- handleDesktopRemoved ' + id)
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


    function debugObject(o) {
        //for (var p in Object.getOwnPropertyNames(o)) {
            //log("========= " + o[p]);
        //}

        var keys = Object.keys(o);
        for(var i=0; i<keys.length; i++) {
            var key = keys[i];
            // prints all properties, signals, functions from object
            log('======== ' + key + ' : ' + o[key]);
        }
    }

    function handleLayoutChanged() {
        log('--------------- layoutChanged')
        if (manager.desktopCount < thumbs.count) {
            // this has been handled by handleDesktopRemoved
        }

        for (var i = 0; i < thumbs.count; i++) {
            var r = manager.calculateDesktopThumbRect(i);
            //log('   ----- ' + (i+1) + ': ' + thumbs.get(i).obj.x + ',' + thumbs.get(i).obj.y + 
                //'  => ' + r.x + ',' + r.y)
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
        for (var i = 1; i <= manager.desktopCount; i++) {
            newDesktop(i)
        }
    }
}

