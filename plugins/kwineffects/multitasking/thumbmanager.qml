import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
import QtQuick.Window 2.0
import com.deepin.kwin 1.0
import QtGraphicalEffects 1.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kwin 2.0 as KWin

Rectangle {
    id: root
    x: 0
    y: 0
    width: manager.containerSize.width;
    height: manager.containerSize.height;
    color: "transparent"

    objectName: "root"

    Rectangle {
        id: background
        x: 0
        y: 0
        height: root.height
        width: root.width
        color: "black"
        opacity: 0.6
    }

    function log(msg) {
        manager.debugLog(msg)
    }

    signal qmlRequestMove2Desktop(int screen, int desktop, var winId);
    signal resetModel();
    signal qmlCloseMultitask();
    signal qmlRemoveWindowThumbnail(int screen, int desktop, var winId);
    signal qmlForceResetDesktopModel();

    Component {
        id: windowThumbnailView;
        Rectangle {
            color: "red";
            function resetWindowThumbnailModel()
            {
                windowThumbnailViewGrid.columns = $Model.getCalculateColumnsCount(screen,desktop);
                windowThumbnailRepeater.model = $Model.windows(screen, desktop);
                windowThumbnailRepeater.update();
            } 
            GridLayout {
                id:windowThumbnailViewGrid
                x: desktopThumbnailWidth/7;
                y: desktopThumbnailHeight/10;
                width: desktopThumbnailWidth*5/7;
                height: desktopThumbnailHeight*8/10;
                columns : $Model.getCalculateColumnsCount(screen,desktop);
                Repeater {
                    id: windowThumbnailRepeater
                    model: $Model.windows(screen, desktop);
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
                        MouseArea{ //zhd add   for drag window
                            id:avatarMousearea
                            anchors.fill:parent
                            drag.target:winAvatar
                            drag.smoothed :true
                            property var pressedTime;
                            property var releaseTime;

                            onPressed: {
                                winAvatar.Drag.hotSpot.x = mouse.x;
                                winAvatar.Drag.hotSpot.y = mouse.y;
                                pressedTime = Date.now(); 
                            }
                            onReleased: {
                                releaseTime = Date.now();
                                if ((releaseTime - pressedTime) < 200) {
                                    $Model.setCurrentIndex(desktop - 1);
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
                        $Model.forceResetModel()
                        //console.log(" model is changed !!!!!!!!!!")
                    }
                }
            }
        }
    }

    Component {
        id: desktopThumbmailView;
        Rectangle {
            property int desktopThumbmailItemWidth: screenWidth/7;
            property int desktopThumbmailItemHeight: screenHeight/6;
            id:wholeDesktopThumbmailView
            width: screenWidth;
            height: parent.height;
            color: "transparent"

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    qmlCloseMultitask();
                }
            }
            ListView {
                id: view
                y:desktopThumbmailItemHeight/8;
                width: 0;
                height: parent.height;
                orientation: ListView.Horizontal;
                model: $Model
                interactive : false;
                clip: true;
//                spacing: desktopThumbmailItemWidth/10

                delegate: Rectangle {

                    id: thumbBackRect;
                    color: "transparent";
                    width: desktopThumbmailItemWidth;
                    height: desktopThumbmailItemHeight;

                    MouseArea {
                        id: thumbBackRectMouseArea
                        anchors.fill: parent;
                        hoverEnabled: true;
                        onEntered: {
                            if ($Model.rowCount() != 1) {
                                closeBtn.visible = true;
                            }
                        }

                        onExited: {
                            closeBtn.visible = false;
                        }
                    }

                    Rectangle {
                        id: closeBtn;
                        anchors.right: parent.right;
                        width: closeBtnIcon.width;
                        height: closeBtnIcon.height;
                        color: "transparent";
                        property int desktop: desktopThumbnail.desktop;
                        visible: false;
                        z:100

                        Image {
                            id: closeBtnIcon;
                            source: "qrc:///icons/data/close_normal.svg"
                        }

                        MouseArea {
                            anchors.fill: closeBtn;
                            onClicked: {
                                $Model.remove(index);
                            }
                        }

                        Connections {
                            target: view;
                            onCountChanged: {
                                closeBtn.visible = false;
                            }
                        }
                    }


                    property bool isDesktopHightlighted: index === $Model.currentDeskIndex

                    Rectangle {
                    id: thumbDelegate
                    width: desktopThumbmailItemWidth - closeBtnIcon.width/4*3
                    height: desktopThumbmailItemHeight - closeBtnIcon.height/4*3
                    color: "transparent"
                    anchors.centerIn: parent

                    DesktopThumbnail {
                        id: desktopThumbnail;
                        desktop: index + 1;
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        property var originParent: view

                        radius: 10
                        width: thumbDelegate.width
                        height: thumbDelegate.height
                        MouseArea {
                            id: desktopThumbMouseArea
                            anchors.fill: parent;
                            hoverEnabled: true;

                            onClicked: {
                                $Model.setCurrentIndex(index);
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
                                if ($Model.rowCount() != 1) {
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
                        }

                        Rectangle {
                            id:winThumrect;
                            width: parent.width;
                            height: parent.height;
                            border.color: "lightskyblue";
                            border.width: 0;
                            color: "transparent";
                            radius: 10;
                        }
                    }

                    DropArea {
                        id: workspaceThumbDrop
                        anchors.fill: parent;
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
                                if (workspaceThumbDrop.designated == drop.source.desktop && drop.source.pendingDragRemove) {
                                        //FIXME: could be a delete operation but need more calculation
                                        log("----------- workspaceThumbDrop: close desktop " + from)
                                        $Model.remove(index);
                                } else {
                                    if (from == to) return
                                    if(drop.source.originParent != originParent) return
                                    log("from:"+from + " to:"+to)
                                    $Model.move(from-1, to-1);
                                    //log("----------- workspaceThumbDrop: reorder desktop ")
                                }
                            }
                            if(drop.keys[0]==="DraggingWindowAvatar" || drop.keys[0]==="DragwindowThumbnailitemdata"){  //zhd add

                                console.log("DraggingWindowAvatar :Droppsource   " +drag.source.draggingdata +"desktop index:" + desktopThumbnail.desktop + "current screen: "+ currentScreen);
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
                }
                }
                //center
                onCountChanged: {
                    view.width = desktopThumbmailItemWidth * count+view.spacing*(count-1);
                    view.x = (parent.width - view.width) / 2;
                    plus.visible = count < 4;
                    setGridviewData();
                    bigWindowThrumbContainer.curdesktop=$Model.currentIndex()+1 //zhd add
                }


                Connections {
                    target: $Model;
                    onCurrentIndexChanged: {
                        setGridviewData();
                        bigWindowThrumbContainer.curdesktop=$Model.currentIndex()+1 //zhd add 
                    }
                }
            }

            Rectangle {
                id: plus
                enabled: visible
                color: "#33ffffff"

                x:screenWidth-desktopThumbmailItemWidth;
                y:desktopThumbmailItemHeight/8+desktopThumbmailItemHeight/2-plus.height/2;

                width: desktopThumbmailItemHeight/2;
                height: desktopThumbmailItemHeight/2;
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

//                Behavior on x {
//                    enabled: animateLayouting
//                    PropertyAnimation { duration: 300; easing.type: Easing.Linear }
//                }

                MouseArea {
                    anchors.fill: parent
                    //hoverEnabled: true
                    onClicked: {
                        $Model.append();
                        $Model.setCurrentIndex($Model.rowCount() - 1);
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
                anchors.right:wholeDesktopThumbmailView.right;
                width: screenWidth-view.x-view.width;
                height: view.height;
                onEntered: console.log("entered")
                keys:['PlusButton']
                onDropped: {
                    var winId = drag.source.winId;
                    $Model.append();
                    var currentDesktop = $Model.rowCount();
                    qmlRequestMove2Desktop(currentScreen, currentDesktop, winId);
                    $Model.setCurrentIndex(currentDesktop - 1);
                }
            }
            //window thumbnail

            function setGridviewData() {
                if ($Model.getDesktopClientCount(currentScreen,$Model.currentIndex()+1) !== 0) {
                    grid.rows = $Model.getCalculateRowCount(currentScreen,$Model.currentIndex()+1)
                    grid.columns = $Model.getCalculateColumnsCount(currentScreen,$Model.currentIndex()+1);
                    windowThumbnail.model = $Model.windows(currentScreen, $Model.currentIndex()+1);
                    //console.log('++++++++++++++++++'+grid.rows+'------------------'+grid.columns)
                    for(var i=0;i<windowThumbnail.count;i++) {
                            var scale = $Model.getWindowHeight(windowThumbnail.itemAt(i).winId)/$Model.getWindowWidth(windowThumbnail.itemAt(i).winId);
                            var calculationwidth= (screenWidth*5/7/grid.columns)*4/5;
                            var calculationheight = calculationwidth*scale;
                            var narrow = 0.8;
                            //console.log('1++++++++++++++++++'+(screenHeight - view.height-35)/grid.rows+'------------------'+calculationheight+grid.rows);
                            while (calculationheight > grid.height/grid.rows) {
                                calculationwidth = calculationwidth * narrow;
                                calculationheight = calculationheight* narrow;
                                //console.log('++++++++++++++++++'+grid.height/grid.rows+'------------------'+calculationheight);
                            }
                            windowThumbnail.itemAt(i).Layout.preferredWidth = calculationwidth;
                            windowThumbnail.itemAt(i).Layout.preferredHeight = calculationheight;
                    }
                }else{
                    grid.rows = $Model.getCalculateRowCount(currentScreen,$Model.currentIndex()+1)
                    grid.columns = $Model.getCalculateColumnsCount(currentScreen,$Model.currentIndex()+1);
                    windowThumbnail.model = $Model.windows(currentScreen, $Model.currentIndex()+1);
                }
                grid.update();
             }
            Rectangle{
                id: bigWindowThrumbContainer
                x: 0
                y: view.y + view.height;
                width: screenWidth  //  other area except grid  can receove
                height: screenHeight - view.height-35;
                color:"transparent"

                property int curdesktop:1
                z:1

                //zhd add for receive window thrumbnail
                DropArea { 
                    id: workspaceThumbnailDropArea
                    anchors.fill: parent
                    keys: ['DragwindowThumbnailitemdata','DraggingWindowAvatar']

                    onDropped: {
                        //console.log("bigWindowThrumbContainer droped");

                        var from = drop.source.desktop

                        if(from!=bigWindowThrumbContainer.curdesktop && bigWindowThrumbContainer.curdesktop!=null && drop.keys[0]==="DraggingWindowAvatar"){

                            console.log("DraggingWindow on big view  :Dropsource:" +drag.source.draggingdata +"  desktop index:" +  bigWindowThrumbContainer.curdesktop+ "  current screen: "+ currentScreen);
                            qmlRequestMove2Desktop(currentScreen,bigWindowThrumbContainer.curdesktop,drag.source.draggingdata);
                            setGridviewData();
                        }
                        if(drop.keys[0] === "DragwindowThumbnailitemdata") {
                            //console.log(currentScreen+"---------"+$Model.currentIndex()+"---------"+drag.source.draggingdata);
                            if (!$Model.isCurrentScreenWindows(currentScreen,$Model.currentIndex()+1, drag.source.draggingdata)) {
                                $Model.moveToScreen(currentScreen,$Model.currentIndex()+1, drag.source.draggingdata);
                                qmlRequestMove2Desktop(currentScreen,$Model.currentIndex()+1,drag.source.draggingdata);
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
                    columns : $Model.getCalculateColumnsCount(currentScreen,$Model.currentIndex()+1);
                    Repeater {
                        id: windowThumbnail;
                        //model: $Model.windows(currentScreen)
                        PlasmaCore.WindowThumbnail {
                            id:windowThumbnailitem
                            property bool isHightlighted: winId == $Model.currentWindowThumbnail;
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth:-1;
                            Layout.preferredHeight:-1;
                            winId: modelData;
                            property var draggingdata: winId
                            property bool  dropreceived:false
                            


                            Drag.keys:["DragwindowThumbnailitemdata", "PlusButton"];
                            Drag.active: false// windowThumbnailitemMousearea.drag.active
                            Drag.hotSpot {
                                x:0
                                y:0
                            }

                            Rectangle {
                                id:backgroundrect;
                                width: parent.width;
                                height: parent.height;
                                border.color: "lightgray";
                                border.width: 0;
                                color: "transparent";
                            }
                          
                            MouseArea {
                                id:windowThumbnailitemMousearea
                                anchors.fill: parent //windowThumbnailitem
                                acceptedButtons: Qt.LeftButton| Qt.RightButton;
                                hoverEnabled: true;
                                property var pressedTime;
                                

                                property int originWidth
                                property int originHeight
                                property int originX
                                property int originY

                               // property var lastPos:0;


                                //drag.target:windowThumbnailitem
                                drag.smoothed :false
                                drag.threshold:0
                                preventStealing:true


                                onEntered: {
                                    $Model.setCurrentSelectIndex(modelData);
                                    closeClientBtn.visible = true;
                                    stickedBtn.visible = true;
                                    if($Model.getWindowKeepAbove(modelData))
                                    {
                                        stickedBtnIcon.source = "qrc:///icons/data/sticked_normal.svg"
                                    }else{
                                        stickedBtnIcon.source = "qrc:///icons/data/unsticked_normal.svg"
                                    }
                                    
                                }
                              
                                //  excute on released
                                // onClicked: {
                                //     $Model.setCurrentSelectIndex(modelData);
                                //     $Model.windowSelected( modelData );

                                // }
                                onExited: {
                                     closeClientBtn.visible = false;
                                     stickedBtn.visible = false;
                                }
                           
                                onPressed:{
                                    //console.log("onPressed x:",windowThumbnailitem.x+"  y:"+windowThumbnailitem.y +" width:"+windowThumbnailitem.width+" height:"+windowThumbnailitem.height+ " mouse.x:" + mouse.x+  "  mouse.y:"+mouse.y)
                                    originWidth = windowThumbnailitem.width
                                    originHeight = windowThumbnailitem.height
                                    originX = windowThumbnailitem.x
                                    originY = windowThumbnailitem.y



                                    var imgheight = $Model.getWindowHeight(windowThumbnailitem.winId);
                                    var imgwidth  =   $Model.getWindowWidth(windowThumbnailitem.winId);

                                   
                                    var scale=1;
                                    if(imgwidth > 0 && imgheight > 0)
                                        scale = imgwidth/imgheight;
                                    else
                                        scale = 0.75

                                    var dragingImgWidth
                                    var dragingImgHeight

                                    if(scale>1){
                                        dragingImgWidth = 120
                                        dragingImgHeight = dragingImgWidth /scale;
                                    }else{
                                        dragingImgHeight = 120
                                        dragingImgWidth = dragingImgHeight * scale;
                                    }


                                    //只缩小，不设置位置，因为大小变了。这个ｍｏｕｓｅ　已经不是正确的位置了。只能下次再取（就是到　ｐｏｓｉｔｉｏｎ　中取得）
                                    windowThumbnailitem.width = dragingImgWidth
                                    windowThumbnailitem.height = dragingImgHeight

                                    pressedTime = Date.now();

                                    closeClientBtn.visible = false;
                                    stickedBtn.visible = false;

                                    windowThumbnailitem.Drag.active=true
                                }

                                
                               
                                //onpress 后，会马上执行　onMouseXChanged，然后再执行　onPositionChanged
                                //此时要先用pressed 做条件，不能使用windowThumbnailitem.Drag.active
                                onMouseXChanged: {
                                    if(pressed){

                                        //绝对坐标方法
                                        var positionInRoot = mapToItem(root, mouse.x, mouse.y) 
                                        windowThumbnailitem.x = (positionInRoot.x - windowThumbnailitem.width / 2);

                                        //console.log("onMouseXChanged x:",windowThumbnailitem.x+"  y:"+windowThumbnailitem.y +"  lastPos.x:"+ lastPos + " posInRoot x:"+positionInRoot.x+" posInRoot y:"+positionInRoot.y+" mouse.x:" + mouse.x+  "  mouse.y:"+mouse.y)

                                        //相对坐标方法
                                        //windowThumbnailitem.x += (mouse.x - windowThumbnailitem.width / 2);

                                    }
                                        
                                }
                                onMouseYChanged: {
                                    if(pressed){
                                        var positionInRoot = mapToItem(root, mouse.x, mouse.y) 
                                        windowThumbnailitem.y = (positionInRoot.y - windowThumbnailitem.height / 2);

                                    }
                                        
                                }
                                onReleased:{
                                    var curtime = Date.now();
                                    if((curtime - pressedTime) < 200){
                                        $Model.setCurrentSelectIndex(modelData);
                                        $Model.windowSelected( modelData );
                                        //
                                    }else{
                                        if(!windowThumbnailitem.Drag.active){
                                            ////恢复现场
                                            windowThumbnailitem.width = originWidth
                                            windowThumbnailitem.height = originHeight
                                            windowThumbnailitem.x = originX
                                            windowThumbnailitem.y = originY
                                            closeClientBtn.visible = true;
                                            stickedBtn.visible = true;
                                        }
                                    }
                                    windowThumbnailitem.Drag.drop()
                                    windowThumbnailitem.Drag.active=false
                                    
                                }
                                /*drag.onActiveChanged: {
                                    if (!windowThumbnailitemMousearea.drag.active) {
                                      //  console.log('------- release on ' + windowThumbnailitemMousearea.drag.target + index)
                                        windowThumbnailitem.Drag.drop();

                                    }else{
                                        //console.log("mouse.x"+mouseDragStart.x+ " mouse.y:"+mouseDragStart.y +" win X: " +windowThumbnailitem.x+" win Y: "+windowThumbnailitem.y);
                                    }
                                }*/
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
                                anchors.right: parent.right;
                                width: closeClientBtnIcon.width;
                                height: closeClientBtnIcon.height;
                                color: "transparent";
                                Image {
                                    id: closeClientBtnIcon;
                                    source: "qrc:///icons/data/close_normal.svg"
                                }
                                MouseArea {
                                    anchors.fill: closeClientBtn;
                                    onClicked: {
                                        qmlRemoveWindowThumbnail(currentScreen,$Model.currentIndex()+1, windowThumbnailitem.winId)
                                    }
                                }
                            }

                            Rectangle {
                                id: stickedBtn;
                                anchors.left: parent.left;
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
                                    onClicked: {
                                        if($Model.getWindowKeepAbove(modelData))
                                        {
                                            stickedBtnIcon.source = "qrc:///icons/data/unsticked_normal.svg"
                                        }else{
                                            stickedBtnIcon.source = "qrc:///icons/data/sticked_normal.svg"
                                        }
                                        $Model.setWindowKeepAbove(modelData);
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
                                x:windowThumbnailitem.width/2 -  clientIconImage.width/2;
                                y:windowThumbnailitem.height - clientIconImage.height;
                                width: clientIconImage.width;
                                height: clientIconImage.height;
                                color: "transparent";
                                Image {
                                    id: clientIconImage;
                                    source: "image://imageProvider/" + modelData ;
                                    cache : false
                                }
                            }
                        }
                    }
                Connections {
                    target: root
                    onResetModel: {
                        setGridviewData();
                        bigWindowThrumbContainer.curdesktop=$Model.currentIndex()+1
                    }
                }
                }
            }
        }
    }
    Component.onCompleted: {
        var numScreens = 1;
        if ($Model.isExtensionMode) {
           numScreens = $Model.numScreens();
        }

        for (var i = 0; i < numScreens; ++i) {
            var geom = $Model.screenGeometry(i);
            var src =
                'import QtQuick 2.0;' +
                'Loader {' +
                '   x: ' + geom.x + ';' +
                '   y: ' + geom.y + ';' +
                '   property int screenWidth: ' + geom.width + ';' +
                '   property int screenHeight: '+ geom.height + ';'+
                '   height: '+ geom.height/5+';'+
                '   property int currentScreen: ' + i + ';' +
                '   sourceComponent: desktopThumbmailView;' +
                '}';
            Qt.createQmlObject(src, root, "dynamicSnippet");
        }
    }
}

