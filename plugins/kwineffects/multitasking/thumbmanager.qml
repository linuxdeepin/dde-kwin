import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
import QtQuick.Window 2.0
import com.deepin.kwin 1.0
import QtGraphicalEffects 1.0
import org.kde.plasma 2.0 as PlasmaCore
import org.kde.kwin 2.0 as KWin

Rectangle {
    id: root
    x: 0
    y: 0
    width: manager.containerSize.width;
    height: manager.containerSize.height;
    color: "transparent"
    objectName: "root"

    function log(msg) {
        manager.debugLog(msg)
    }

    signal qmlRequestMove2Desktop(int screen, int desktop, var winId);
    signal resetModel();
    signal qmlCloseMultitask();
    signal qmlRemoveWindowThumbnail(int screen, int desktop, var winId);
    signal qmlForceResetDesktopModel();
    signal qmlUpdateDesktopThumBackground();
    signal qmlUpdateBackground();
    signal qmlRequestGetBackground(int desktop, int monitor,int width,int height);

    Component {
        id: windowThumbnailView;
        Rectangle {
            color: "red";
            function resetWindowThumbnailModel()
            {
                windowThumbnailViewGrid.columns = multitaskingModel.getCalculateColumnsCount(screen,desktop);
                windowThumbnailRepeater.model = multitaskingModel.windows(screen, desktop);
                windowThumbnailRepeater.update();
            } 
            GridLayout {
                id:windowThumbnailViewGrid
                x: desktopThumbnailWidth/7 + closeBtnWidth/8 + 7;
                y: desktopThumbnailHeight/10 + closeBtnHeight/8 + 7;
                width: desktopThumbnailWidth*5/7 - closeBtnWidth/4 - 14;
                height: desktopThumbnailHeight*8/10 - closeBtnHeight/4 - 14;

                columns : multitaskingModel.getCalculateColumnsCount(screen,desktop);
                Repeater {
                    id: windowThumbnailRepeater
                    model: multitaskingModel.windows(screen, desktop);
                    PlasmaCore.WindowThumbnail {
                        Layout.fillWidth: true;
                        Layout.fillHeight: true;
                        winId: modelData;

                        //zhd add
                        id:winAvatar
                        property var draggingdata: winId
                        property bool dropreceived : false

                        property int dragingIndex:index
                        Drag.keys: ["DraggingWindowAvatar"];  //for holdhand
                        Drag.active:  avatarMousearea.drag.active
                        Drag.hotSpot {
                            x: width/2
                            y: height/2
                        }
                        MouseArea { //zhd add   for drag window
                            id:avatarMousearea
                            anchors.fill:parent
                            drag.target:winAvatar
                            drag.smoothed :true
                            property var pressedTime;
                            property var releaseTime;

                            Accessible.role: Accessible.Button
                            Accessible.name: "Ma_winThumb_small_" +desktop+"_"+screen+"_"+ winId 
                            Accessible.description: "small windowthumbnail_desktop_screen_winId"
                            Accessible.onPressAction:pressed()

                            onPressed: {
                                winAvatar.Drag.hotSpot.x = mouse.x;
                                winAvatar.Drag.hotSpot.y = mouse.y;
                                pressedTime = Date.now(); 
                            }
                            onReleased: {
                                releaseTime = Date.now();
                                if ((releaseTime - pressedTime) < 200) {
                                    multitaskingModel.setCurrentIndex(desktop - 1);
                                }
                            }
                            drag.onActiveChanged: {
                                if (!avatarMousearea.drag.active) {
                                    console.log('------- release on ' + avatarMousearea.drag.target)
                                    winAvatar.Drag.drop();
                                }
                            }
                            states: State {
                                when: avatarMousearea.drag.active;
                                ParentChange {
                                    target: winAvatar;
                                    parent: root;
                                }

                                PropertyChanges {
                                    target: winAvatar;
                                    z: 100;

                                }
                                // AnchorChanges {
                                //     target: winAvatar;
                                //     anchors.horizontalCenter: undefined
                                //     anchors.verticalCenter: undefined
                                // }
                            }
                        }
                        //zhd add end
                    }
                }
                Connections {
                    target: root
                    onResetModel: {
                        resetWindowThumbnailModel()
                        //console.log(" model is changed !!!!!!!!!!")
                    }
                }
                Connections {
                    target: root
                    onQmlForceResetDesktopModel: {
                        resetWindowThumbnailModel()
                        multitaskingModel.forceResetModel()
                        //console.log(" model is changed !!!!!!!!!!")
                    }
                }
            }
        }
    }

    Component {
        id: desktopThumbnailView;
        Rectangle {
            Rectangle {
                id: mostBigbackgroundRect
                width: screenWidth;
                height: screenHeight;
                Image {
                    id: backgroundImage;
                    source: "image://BackgroundImageProvider/" + (currentIndex+1 + "/" + screenname);
                    cache : false
                }
                FastBlur {
                    anchors.fill: backgroundImage
                    source: backgroundImage
                    radius: 70
                }
                Connections {
                    target: root
                    onQmlUpdateBackground: {
                        backgroundImage.source = "";
                        backgroundImage.source = "image://BackgroundImageProvider/" + (multitaskingModel.currentIndex()+1 + "/" + screenname);
                    }
                }
            }

            property int desktopThumbnailItemWidth: screenWidth/7;
            property int desktopThumbnailItemHeight: screenHeight/6;
            id:wholeDesktopThumbnailView
            width: screenWidth;
            height: parent.height;
            color: "transparent"

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    qmlCloseMultitask();
                }
                Accessible.role: Accessible.Button
                Accessible.name: "Ma_background_1" 
                Accessible.description: "background of whole desktopThumbnail area"
                Accessible.onPressAction: pressed()
            }
            ListView {
                id: view
                y:desktopThumbnailItemHeight/8;
                width: 0;
                height: parent.height;
                orientation: ListView.Horizontal;
                model: multitaskingModel
                interactive : false;
                clip: true;

                delegate: Rectangle {

                    id: thumbDelegate;
                    width: desktopThumbnailItemWidth;
                    height: desktopThumbnailItemHeight;
                    color: "transparent"

                    property bool isDesktopHightlighted: index === multitaskingModel.currentDeskIndex

                    Rectangle {
                        id: desktopThumbnail;
                        property int desktop: smalldesktopThumbnail.desktop;
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        property var originParent: view
                        color: "transparent"

                        radius: 10
                        width: thumbDelegate.width
                        height: thumbDelegate.height

                        DesktopThumbnail {
                            id : smalldesktopThumbnail
                            desktop: index + 1;
                            width: parent.width - closeBtn.width*(3/4)
                            height: parent.height - closeBtn.height*(3/4)
                            anchors.centerIn: parent
                            radius: 10
                            monitor: screenname

                            Rectangle {
                                id:winThumrect;
                                width: parent.width;
                                height: parent.height;
                                border.color: "lightskyblue";
                                border.width: 0;
                                color: "transparent";
                                radius: 10;
                            }
                            Component.onCompleted: {
                                smalldesktopThumbnail.getDesktopThumbnailBackground(desktop,monitor,width,height);
                            }
                            Connections {
                                target: root
                                onQmlUpdateDesktopThumBackground: {
                                    smalldesktopThumbnail.getDesktopThumbnailBackground(smalldesktopThumbnail.desktop,smalldesktopThumbnail.monitor,smalldesktopThumbnail.width,smalldesktopThumbnail.height);
                                }
                            }
                        }


                        Rectangle {
                            id: closeBtn;
                            x: thumbDelegate.width - closeBtn.width
                            y: 0
                            z: 100
                            width: closeBtnIcon.width
                            height: closeBtnIcon.height
                            color: "transparent";

                            property int desktop: desktopThumbnail.desktop;
                            visible: false;

                            Image {
                                id: closeBtnIcon;
                                source: "qrc:///icons/data/close_normal.svg"
                            }


                            Connections {
                                target: view;
                                onCountChanged: {
                                    closeBtn.visible = false;
                                }
                            }
                            MouseArea {
                                anchors.fill: parent;
                                onClicked: {
                                    multitaskingModel.remove(index);
                                }
                                Accessible.role: Accessible.Button
                                Accessible.name: "Ma_deskThumb_closeBtn_"+(index+1)+"_"+currentScreen
                                Accessible.description: "desktopThumbnail_closeButton_desktop_screen"
                                Accessible.onPressAction: pressed()
                            }
                        }



                        MouseArea {
                            id: desktopThumbMouseArea
                            anchors.fill: parent;
                            hoverEnabled: true;

                            Accessible.role: Accessible.Button
                            Accessible.name: "Ma_deskThumb_"+(index+1)+"_"+currentScreen
                            Accessible.description: "desktopThumbnail_desktop_screen"
                            Accessible.onPressAction: pressed()

                            onClicked: {
                                multitaskingModel.setCurrentIndex(index);
                            }

                            drag.target: desktopThumbnail;
                            onReleased: {
                                if (manager.desktopCount == 1) {
                                    return
                                }
                            }
                            onPressed: {
                                desktopThumbnail.Drag.hotSpot.x = mouse.x;
                                desktopThumbnail.Drag.hotSpot.y = mouse.y;
                            }
                            drag.onActiveChanged: {
                                if (!desktopThumbMouseArea.drag.active) {
                                    log('------- release ws on ' + thumbDelegate.Drag.target)
                                    desktopThumbnail.Drag.drop();
                                }
                            }

                            onEntered: {
                                if (multitaskingModel.rowCount() !== 1) {
                                    closeBtn.visible = true;
                                }
                            }

                            onExited: {
                                closeBtn.visible = false;
                            }
                        }
                        property bool pendingDragRemove: false
                        Drag.keys: ["workspaceThumb"];
                        Drag.active: view.count > 1 && desktopThumbMouseArea.drag.active
                        Drag.hotSpot {
                            x: width/2
                            y: height/2
                        }

                        states: [State {
                                when: desktopThumbnail.Drag.active;
                                ParentChange {
                                    target: desktopThumbnail;
                                    parent: root;
                                }

                                PropertyChanges {
                                    target: desktopThumbnail;
                                    z: 100;
                                }
                                AnchorChanges {
                                    target: desktopThumbnail;
                                    anchors.horizontalCenter: undefined
                                    anchors.verticalCenter: undefined
                                }
                            },
                            State {
                                name: "isDesktopHightlighted"
                                when: isDesktopHightlighted
                                PropertyChanges {
                                    target: winThumrect
                                    border.width: 3;
                                }
                            }]



                        //window thumbnail
                        Loader {
                            id: winThumLoader
                            sourceComponent: windowThumbnailView
                            property int thumbnailWidth: 50;
                            property int thumbnailHeight: 50;
                            property int screen: currentScreen;
                            property int desktopThumbnailWidth:desktopThumbnail.width
                            property int desktopThumbnailHeight:desktopThumbnail.height
                            property int desktop: desktopThumbnail.desktop;

                            property int closeBtnWidth:closeBtn.width
                            property int closeBtnHeight:closeBtn.height
                        }

//                        Rectangle {
//                            id: closeBtn;
////                            anchors.right: parent.right;
//                            x: parent.width - (parent.width - parent.width*0.85)/2 - closeBtnIcon.width/2
//                            y: (parent.height - parent.height*0.85)/2 - closeBtnIcon.height/2
//                            color: "transparent";
//                            property int desktop: desktopThumbnail.desktop;
//                            visible: false;

//                            Image {
//                                id: closeBtnIcon;
//                                source: "qrc:///icons/data/close_normal.svg"
//                            }

//                            MouseArea {
//                                anchors.fill: closeBtn;
//                                onClicked: {
//                                    multitaskingModel.remove(index);
//                                }
//                            }

//                            Connections {
//                                target: view;
//                                onCountChanged: {
//                                    closeBtn.visible = false;
//                                }
//                            }
//                        }

//                        Rectangle {
//                            id:winThumrect;
//                            width: parent.width;
//                            height: parent.height;
//                            border.color: "lightskyblue";
//                            border.width: 0;
//                            color: "transparent";
//                            radius: 10;
//                        }
                    }

                    DropArea {
                        id: workspaceThumbDrop
                        anchors.fill: parent;
                        anchors.topMargin: -workspaceThumbDrop.parent.parent.parent.y
                        property int designated: index + 1;
                        property var originParent: view

                        z: 1
                        keys: ['workspaceThumb','DraggingWindowAvatar','DragwindowThumbnailitemdata']  //  zhd change for drop a window


                        onDropped: {
                            /* NOTE:
                            * during dropping, PropertyChanges is still in effect, which means
                            * drop.source.parent should not be Loader
                            * and drop.source.z == 100
                            */
                            log("----------- workspaceThumb onDrop")

                            if (drop.keys[0] === 'workspaceThumb') {
                                var from = drop.source.desktop
                                var to = workspaceThumbDrop.designated
                                if (workspaceThumbDrop.designated === drop.source.desktop && drop.source.pendingDragRemove) {
                                        //FIXME: could be a delete operation but need more calculation
                                        log("----------- workspaceThumbDrop: close desktop " + from)
                                        multitaskingModel.remove(index);
                                } else {
                                    if (from === to) {
                                        return;
                                    }
                                    if (drop.source.originParent !== originParent) {
                                        return;
                                    }
                                    log("from:"+from + " to:"+to)
                                    multitaskingModel.move(from-1, to-1);
                                }
                            }
                            if (drop.keys[0] === "DraggingWindowAvatar" || drop.keys[0] === "DragwindowThumbnailitemdata") {  //zhd add

                                log("DraggingWindowAvatar :Droppsource   " +drag.source.draggingdata +"desktop index:" + desktopThumbnail.desktop + "current screen: "+ currentScreen);
                                qmlRequestMove2Desktop(currentScreen,desktopThumbnail.desktop,drag.source.draggingdata);
                                setGridviewData();
                                drag.source.dropreceived=true
                            }
                        }

                        onEntered: {
                            if (drag.keys[0] === 'workspaceThumb') {
                             
                            }
                            //console.log('------[workspaceThumbDrop]: Enter ' + workspaceThumbDrop.designated + ' from ' + drag.source + ', keys: ' + drag.keys + ', accept: ' + drag.accepted)
                        }

                        onExited: {
                            //console.log("----------- workspaceThumb onExited")
                            if (drag.source.pendingDragRemove) {
                                hint.visible = false
                                drag.source.pendingDragRemove = hint.visible
                            }

                        }

                        onPositionChanged: {
                            if (drag.keys[0] === 'workspaceThumb') {
                                var diff = workspaceThumbDrop.parent.y - drag.source.y
                       //         log('------ ' + workspaceThumbDrop.parent.y + ',' + drag.source.y + ', ' + diff + ', ' + drag.source.height/2)
                                if (diff > 0 && diff > drag.source.height/3) {
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
                            color: "transparent"

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
//                }
                }
                //center
                onCountChanged: {
                    view.width = desktopThumbnailItemWidth * count+view.spacing * (count-1);
                    view.x = (parent.width - view.width) / 2;
                    plus.visible = count < 4;
                    setGridviewData();
                    bigWindowThrumbContainer.curdesktop=multitaskingModel.currentIndex()+1 //zhd add
                }


                Connections {
                    target: multitaskingModel;
                    onCurrentIndexChanged: {
                        setGridviewData();
                        bigWindowThrumbContainer.curdesktop=multitaskingModel.currentIndex()+1 //zhd add
                    }
                }
            }

            Rectangle {
                id: plus
                enabled: visible
                color: "#33ffffff"

                x:screenWidth - desktopThumbnailItemWidth;
                y:desktopThumbnailItemHeight/8 + desktopThumbnailItemHeight/2 - plus.height/2;

                width: desktopThumbnailItemHeight/2;
                height: desktopThumbnailItemHeight/2;
                radius: width > 120 ? 30: 15

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

                MouseArea {
                    anchors.fill: parent
                    //hoverEnabled: true

                    Accessible.role: Accessible.Button
                    Accessible.name: "Ma_plusBtn"
                    Accessible.description: "plus Button"
                    Accessible.onPressAction: pressed()

                    onClicked: {
                        multitaskingModel.append();
                        multitaskingModel.setCurrentIndex(multitaskingModel.rowCount() - 1);
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
            DropArea {
                anchors.right:wholeDesktopThumbnailView.right;
                width: screenWidth-view.x-view.width;
                height: view.height;
                onEntered: console.log("entered")
                keys:['PlusButton']
                onDropped: {
                    var winId = drag.source.winId;
                    multitaskingModel.append();
                    var currentDesktop = multitaskingModel.rowCount();
                    qmlRequestMove2Desktop(currentScreen, currentDesktop, winId);
                    multitaskingModel.setCurrentIndex(currentDesktop - 1);
                }
            }
            //window thumbnail

            function setGridviewData() {
               if (multitaskingModel.getDesktopClientCount(currentScreen,multitaskingModel.currentIndex()+1) !== 0) {

                    grid.rows = multitaskingModel.getCalculateRowCount(currentScreen,multitaskingModel.currentIndex()+1)
                    grid.columns = multitaskingModel.getCalculateColumnsCount(currentScreen,multitaskingModel.currentIndex()+1);
                    windowThumbnail.model = multitaskingModel.windows(currentScreen, multitaskingModel.currentIndex()+1);

                    for (var i=0; i < windowThumbnail.count; i++) {

                        var scale = multitaskingModel.getWindowHeight(windowThumbnail.itemAt(i).winId)/multitaskingModel.getWindowWidth(windowThumbnail.itemAt(i).winId);
                        var calculationwidth= (screenWidth*5/7/grid.columns)*4/5;
                        var calculationheight = calculationwidth * scale;
                        var narrow = 0.8;
                        while (calculationheight > grid.height/grid.rows) {
                           calculationwidth = calculationwidth * narrow;
                           calculationheight = calculationheight * narrow;
                        }

                        windowThumbnail.itemAt(i).Layout.preferredWidth = calculationwidth;
                        windowThumbnail.itemAt(i).Layout.preferredHeight = calculationheight;
                    }
                } else {
                    grid.rows = multitaskingModel.getCalculateRowCount(currentScreen,multitaskingModel.currentIndex()+1)
                    grid.columns = multitaskingModel.getCalculateColumnsCount(currentScreen,multitaskingModel.currentIndex()+1);
                    windowThumbnail.model = multitaskingModel.windows(currentScreen, multitaskingModel.currentIndex()+1);
                }
                grid.update();
            }
            Rectangle{
                id: bigWindowThrumbContainer
                x: 0
                y: view.y + view.height;
                width: screenWidth  //  other area except grid  can receove
                height: screenHeight - view.height - 35;
                color:"transparent"

                property int curdesktop:1

                //zhd add for receive window thrumbnail
                DropArea { 
                    id: workspaceThumbnailDropArea
                    anchors.fill: parent
                    keys: ['DragwindowThumbnailitemdata','DraggingWindowAvatar']

                    onDropped: {
                        //console.log("bigWindowThrumbContainer droped");

                        var from = drop.source.desktop

                        if (from !== bigWindowThrumbContainer.curdesktop && bigWindowThrumbContainer.curdesktop!=null && drop.keys[0] === "DraggingWindowAvatar"){

                            console.log("DraggingWindow on big view  :Dropsource:" +drag.source.draggingdata +"  desktop index:" +  bigWindowThrumbContainer.curdesktop+ "  current screen: "+ currentScreen);
                            qmlRequestMove2Desktop(currentScreen,bigWindowThrumbContainer.curdesktop,drag.source.draggingdata);
                            setGridviewData();
                        }
                        if (drop.keys[0] === "DragwindowThumbnailitemdata") {
                            //console.log(currentScreen+"---------"+multitaskingModel.currentIndex()+"---------"+drag.source.draggingdata);
                            if (!multitaskingModel.isCurrentScreenWindows(currentScreen,multitaskingModel.currentIndex()+1, drag.source.draggingdata)) {
                                multitaskingModel.moveToScreen(currentScreen,multitaskingModel.currentIndex()+1, drag.source.draggingdata);
                                qmlRequestMove2Desktop(currentScreen,multitaskingModel.currentIndex()+1,drag.source.draggingdata);
                                setGridviewData();
                            }
                        }
                    }
                    onEntered: {
                        drag.accepted=true;
                        //console.log("bigWindowThrumbContainer enter");
                    }
                }
                //zhd add end

                MouseArea {
                    anchors.fill: parent

                    Accessible.role: Accessible.Button
                    Accessible.name: "Ma_background_2"
                    Accessible.description: "background of windows thumbnail area"
                    Accessible.onPressAction: pressed()

                    onClicked: {
                       qmlCloseMultitask();
                      // console.log("click to close multimask ")
                    }
                }
                GridLayout {
                    id:grid
                    width: screenWidth*5/7;
                    height: screenHeight - view.height-35;
                    anchors.centerIn: parent;
                    columns : multitaskingModel.getCalculateColumnsCount(currentScreen,multitaskingModel.currentIndex()+1);
                    Repeater {
                        id: windowThumbnail;
                        //model: multitaskingModel.windows(currentScreen)

                        Rectangle {
                            id:windowThumbnailitem
                            color: "transparent"
                            property bool isHightlighted: winId == multitaskingModel.currentWindowThumbnail;
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth:-1;
                            Layout.preferredHeight:-1;
                            property var winId: plasmaCoreWindowThumbnail.winId;
                            property var draggingdata: winId
                            property bool  dropreceived:false

                            Drag.keys:["DragwindowThumbnailitemdata", "PlusButton"];
                            Drag.active: false// windowThumbnailitemMousearea.drag.active
                            Drag.hotSpot {
                                x:0
                                y:0
                            }

                            PlasmaCore.WindowThumbnail {
                                id : plasmaCoreWindowThumbnail
                                winId: modelData
                                width: parent.width * 0.85
                                height: parent.height * 0.85
                                anchors.centerIn: parent

                                Rectangle {
                                    id:backgroundrect;
                                    width: parent.width;
                                    height: parent.height;
                                    border.color: "lightgray";
                                    border.width: 0;
                                    color: "transparent";
                                }

                            }

//                            Rectangle {
//                                id:backgroundrect;
//                                width: parent.width;
//                                height: parent.height;
//                                border.color: "lightgray";
//                                border.width: 0;
//                                color: "transparent";
//                            }
                          
                            MouseArea {
                                id:windowThumbnailitemMousearea
                                anchors.fill: parent //windowThumbnailitem
                                acceptedButtons: Qt.LeftButton| Qt.RightButton;
                                hoverEnabled: true;
                                property var pressedTime;

                                Accessible.role: Accessible.Button
                                Accessible.name: "Ma_winThumb_"+(multitaskingModel.currentIndex()+1) + "_" + currentScreen + "_" + winId
                                Accessible.description: "windowThumbnail_desktop_screen_winId"
                                Accessible.onPressAction: pressed()

                                property int originWidth
                                property int originHeight
                                property int originX
                                property int originY
                                property int originMouseX
                                property int originMouseY

                                property bool enableCalcThumbnailGeometry: false

                               // property var lastPos:0;


                                //drag.target:windowThumbnailitem
                                drag.smoothed :false
                                drag.threshold:0
                                preventStealing:true


                                onEntered: {
                                    if (windowThumbnailitem.Drag.active && pressed) {
                                        return;
                                    }
                                    multitaskingModel.setCurrentSelectIndex(modelData);
                                    closeClientBtn.visible = true;
                                    stickedBtn.visible = true;
                                    if (multitaskingModel.getWindowKeepAbove(modelData)) {
                                        stickedBtnIcon.source = "qrc:///icons/data/sticked_normal.svg"
                                    } else {
                                        stickedBtnIcon.source = "qrc:///icons/data/unsticked_normal.svg"
                                    }
                                }
                              
                                //  excute on released
                                // onClicked: {
                                //     multitaskingModel.setCurrentSelectIndex(modelData);
                                //     multitaskingModel.windowSelected( modelData );

                                // }
                                onExited: {
                                     closeClientBtn.visible = false;
                                     stickedBtn.visible = false;
                                }
                                Timer {
                                    id: pressedTimer
                                    interval: 200; running: false; repeat: false
                                    onTriggered: {
                                        windowThumbnailitemMousearea.enableCalcThumbnailGeometry = true
                                    }
                                }
                           
                                onPressed: {
                                     originWidth = windowThumbnailitem.width
                                     originHeight = windowThumbnailitem.height
                                     originX = windowThumbnailitem.x
                                     originY = windowThumbnailitem.y
                                     enableCalcThumbnailGeometry = false
                                     originMouseX = mouse.x
                                     originMouseY = mouse.y

                                     if (!pressedTimer.running) {
                                         pressedTimer.start()
                                     }

                                     closeClientBtn.visible = false;
                                     stickedBtn.visible = false;
                                }
                                function calcDragingThumbnailGeometry() {

                                    if (!enableCalcThumbnailGeometry) {
                                        return ;
                                    }

                                    var dragingImgWidth
                                    var dragingImgHeight
                                    var imgheight = multitaskingModel.getWindowHeight(windowThumbnailitem.winId);
                                    var imgwidth = multitaskingModel.getWindowWidth(windowThumbnailitem.winId);
                                    var scale = 1;

                                    enableCalcThumbnailGeometry = false

                                    if (imgwidth > 0 && imgheight > 0) {
                                        scale = imgwidth/imgheight;
                                    } else {
                                        scale = 0.75
                                    }
                                    if (scale>1) {
                                        dragingImgWidth = 120
                                        dragingImgHeight = dragingImgWidth /scale;
                                    } else {
                                        dragingImgHeight = 120
                                        dragingImgWidth = dragingImgHeight * scale;
                                    }
                                    //只缩小，不设置位置，因为大小变了。这个ｍｏｕｓｅ　已经不是正确的位置了。只能下次再取（就是到　ｐｏｓｉｔｉｏｎ　中取得）
                                    windowThumbnailitem.width = dragingImgWidth
                                    windowThumbnailitem.height = dragingImgHeight

                                    windowThumbnailitem.Drag.active = true
                                }

                                //onpress 后，会马上执行　onMouseXChanged，然后再执行　onPositionChanged
                                //此时要先用pressed 做条件，不能使用windowThumbnailitem.Drag.active
                                onMouseXChanged: {
                                    calcDragingThumbnailGeometry()

                                    if (pressed && !pressedTimer.running) { 
                                        //绝对坐标方法
                                        var mousePosInRoot = mapToItem(root, mouse.x, mouse.y) 
                                        windowThumbnailitem.x = (mousePosInRoot.x - windowThumbnailitem.width / 2);
                                    }
                                        
                                }
                                onMouseYChanged: {
                                    calcDragingThumbnailGeometry()

                                    if (pressed && !pressedTimer.running) {
                                        var mousePosInRoot = mapToItem(root, mouse.x, mouse.y) 
                                        windowThumbnailitem.y = (mousePosInRoot.y - windowThumbnailitem.height / 2);
                                    }
                                }
                                onReleased: {
                                    if (pressedTimer.running) {
                                        pressedTimer.stop()
                                        multitaskingModel.setCurrentSelectIndex(modelData);
                                        if (Math.abs(originMouseX-mouse.x)<20 && Math.abs(originMouseY-mouse.y)<20) {
                                            multitaskingModel.windowSelected(modelData);
                                        }
                                    } else {
                                        if (!windowThumbnailitem.Drag.active || enableCalcThumbnailGeometry) {
                                            ////恢复现场
                                            windowThumbnailitem.width = originWidth
                                            windowThumbnailitem.height = originHeight
                                            windowThumbnailitem.x = originX
                                            windowThumbnailitem.y = originY
                                            closeClientBtn.visible = true;
                                            stickedBtn.visible = true;
                                            enableCalcThumbnailGeometry = false
                                        }
                                    }
                                    windowThumbnailitem.Drag.drop()
                                    windowThumbnailitem.Drag.active = false
                                }
                             
                                states: [
                                    State {
                                    when: windowThumbnailitem.Drag.active;
                                    ParentChange {
                                        target: windowThumbnailitem;
                                        parent: root;
                                    }
                                    PropertyChanges {
                                        target: windowThumbnailitem;
                                        z: 100;
                                    }
                                    // PropertyChanges{
                                    //     target:windowThumbnailitemMousearea
                                    //     width:120
                                    //     height:80
                                    // }
                                    // AnchorChanges{
                                    //     target: windowThumbnailitem;

                                    //     Layout.fillWidth: false
                                    //     Layout.fillHeight:false
                                    // }
                                   
                                }]
                            }
                            Rectangle {
                                id: closeClientBtn;
                                visible:false;
                                x: parent.width - (parent.width - parent.width*0.85)/2 - closeClientBtnIcon.width/2
                                y: (parent.height - parent.height*0.85)/2 - closeClientBtnIcon.height/2
//                                anchors.right: parent.right;
                                width: closeClientBtnIcon.width;
                                height: closeClientBtnIcon.height;
                                color: "transparent";
                                Image {
                                    id: closeClientBtnIcon;
                                    source: "qrc:///icons/data/close_normal.svg"
                                }
                                MouseArea {
                                    anchors.fill: closeClientBtn;
                                    Accessible.role: Accessible.Button
                                    Accessible.name: "Ma_winThumb_closeBtn_"+(multitaskingModel.currentIndex()+1)+"_"+currentScreen+"_"+ windowThumbnailitem.winId
                                    Accessible.description: "windowThumbnail_closeButton_desktop_screen_winId"
                                    Accessible.onPressAction: pressed()
                                    onClicked: {
                                        qmlRemoveWindowThumbnail(currentScreen,multitaskingModel.currentIndex()+1, windowThumbnailitem.winId)
                                    }
                                }
                            }

                            Rectangle {
                                id: stickedBtn;
                                x: (parent.width - parent.width*0.85)/2 - stickedBtn.width/2
                                y: (parent.height - parent.height*0.85)/2 - stickedBtn.height/2
//                                anchors.left: parent.left;
                                width: stickedBtnIcon.width;
                                height: stickedBtnIcon.height;
                                color: "transparent";
                                visible:false;

                                Image {
                                    id: stickedBtnIcon;
                                    source: "qrc:///icons/data/unsticked_normal.svg"
                                }

                                MouseArea {
                                    anchors.fill: stickedBtn;

                                    Accessible.role: Accessible.Button
                                    Accessible.name: "Ma_winThumb_stickedBtn_"+(multitaskingModel.currentIndex()+1)+"_"+currentScreen+"_" + windowThumbnailitem.winId
                                    Accessible.description: "windowThumbnai_stickedButton_desktop_screen_winId"
                                    Accessible.onPressAction: pressed()

                                    onClicked: {
                                        if (multitaskingModel.getWindowKeepAbove(modelData))
                                        {
                                            stickedBtnIcon.source = "qrc:///icons/data/unsticked_normal.svg"
                                        } else {
                                            stickedBtnIcon.source = "qrc:///icons/data/sticked_normal.svg"
                                        }
                                        multitaskingModel.setWindowKeepAbove(modelData);
                                    }
                                }

                            }

                            states: State {
                                name: "isHightlighted"
                                when: isHightlighted 
                                PropertyChanges {
                                    target: windowThumbnailitem
                                    scale: 1.02
                                    
                                }
                                PropertyChanges {
                                    target: backgroundrect
                                    border.width: 5;
                                }
                            }

                            Rectangle {
                                id: clientIcon;
//                                color: "red"
                                x:(plasmaCoreWindowThumbnail.width/2 -  clientIconImage.width/2)/0.85;
                                y: (parent.height - clientIconImage.height/2) - ( parent.height - plasmaCoreWindowThumbnail.height )/2
                                width: clientIconImage.width;
                                height: clientIconImage.height;
                                color: "transparent";
                                Image {
                                    id: clientIconImage;
                                    source: "image://imageProvider/" + modelData ;
                                    cache : false
                                }
                            }
                        } //this
                    }
                Connections {
                    target: root
                    onResetModel: {
                        setGridviewData();
                        bigWindowThrumbContainer.curdesktop=multitaskingModel.currentIndex()+1
                    }
                }
                }
            }
        }
    }
    Component.onCompleted: {
        var numScreens = 1;
        if (multitaskingModel.isExtensionMode) {
           numScreens = multitaskingModel.numScreens();
        }

        for (var i = 0; i < numScreens; ++i) {
            var geom = multitaskingModel.screenGeometry(i);
            var src =
                'import QtQuick 2.0;' +
                'Loader {' +
                '   x: ' + geom.x + ';' +
                '   y: ' + geom.y + ';' +
                '   property int screenWidth: ' + geom.width + ';' +
                '   property int screenHeight: '+ geom.height + ';'+
                '   height: '+ geom.height/5+';'+
                '   property int currentScreen: ' + i + ';' +
                '   sourceComponent: desktopThumbnailView;' +
                '   property var screenname: multitaskingModel.screenName(x,y);' +
                '   property int currentIndex: multitaskingModel.currentIndex();' +
                '}';
            Qt.createQmlObject(src, root, "dynamicSnippet");
        }
    }
}

