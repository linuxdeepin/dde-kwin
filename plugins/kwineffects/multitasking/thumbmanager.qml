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
	width: Screen.width;
	height: Screen.height;
    color: "transparent"

    Rectangle {
        id: background
        x: 0
        y: 0
        height: root.height
        width: {
            var allWitdh = 0;
            for (var i = 0; i < $Model.numScreens(); ++i) {
                var geom = $Model.screenGeometry(i);
                allWitdh += geom.width;
            }
            return allWitdh;
        }
        color: "black"
        opacity: 0.6
    }

    function log(msg) {
        manager.debugLog(msg)
    }

    signal qmlRequestMove2Desktop(int screen, int desktop, var winId);

	Component {
		id: windowThumbnailView;
		Rectangle {
			color: "red";
			Grid {
				Repeater {

					model: $Model.windows(screen, desktop);
					PlasmaCore.WindowThumbnail {
						width: thumbnailWidth;
						height: thumbnailHeight;
						winId: modelData;
                        
                        //zhd add 
                        id:winAvatar  
                        property var draggingdata: winId
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
                            
							onPressed: {
                                 winAvatar.Drag.hotSpot.x = mouse.x;
                                 winAvatar.Drag.hotSpot.y = mouse.y;
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
			}
      	}
	}

	Component {
        id: desktopThumbmailView;
        Rectangle {
            width: screenWidth; 
            height: parent.height;
            color: "transparent"
            ListView {
                id: view
                width: 0;
                height: parent.height;
                orientation: ListView.Horizontal;
                model: $Model
                interactive : false;
                clip: true;

                


                delegate: Rectangle {
                    id: thumbDelegate;
                    width: manager.thumbSize.width;
                    height: manager.thumbSize.height;
                    color: "transparent"
                    
					DesktopThumbnail {
						id: desktopThumbnail;
						desktop: index + 1; 
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter

                        width: thumbDelegate.width
                        height: thumbDelegate.height
                        MouseArea {
                            id: desktopThumbMouseArea
                            anchors.fill: parent;
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
                        }
                        property bool pendingDragRemove: false
                        Drag.keys: ["workspaceThumb"];
                        Drag.active: manager.desktopCount > 1 && desktopThumbMouseArea.drag.active 
                        Drag.hotSpot {
                            x: width/2
                            y: height/2
                        }
                    
                        states: State {
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
                        }

						//window thumbnail
						Loader {
							sourceComponent: windowThumbnailView	
							property int thumbnailWidth: 50;
							property int thumbnailHeight: 50;
							property int screen: currentScreen; 
							property int desktop: desktopThumbnail.desktop;
						}

	                    Rectangle {
							id: closeBtn;
							anchors.right: parent.right;
							width: closeBtnIcon.width;
							height: closeBtnIcon.height;
							color: "transparent";
							property int desktop: desktopThumbnail.desktop;

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
									closeBtn.visible = ($Model.rowCount() != 1);
								}
							}
						}
					}

                    DropArea {
                        id: workspaceThumbDrop
                        anchors.fill: parent;
                        // width: manager.thumbSize.width
                        // height: manager.thumbSize.height
                        property int designated: index + 1;

                        z: 1
                        keys: ['workspaceThumb','DraggingWindowAvatar']  //  zhd change for drop a window
                       

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
                                     if (from == to) {
                                           return
                                      }
                                       console.log("----------- workspaceThumbDrop: reorder desktop ")
                                }
                            }
                            if(drop.keys[0]==="DraggingWindowAvatar"){  //zhd add 

                                
                                //var inx=view.indexAt(Qt.point(drag.x,drag.y));

                                

                                
                                //console.log("DraggingWindowAvatar :Droppsource   " +drag.source.draggingdata +"desktop index:" + desktopThumbnail.desktop + "current screen: "+ currentScreen);
                                qmlRequestMove2Desktop(currentScreen,desktopThumbnail.desktop,drag.source.draggingdata);
                            }
                        }

                        onEntered: {
                            if (drag.keys[0] === 'workspaceThumb') {
                                log('------[workspaceThumbDrop]: Enter ' + workspaceThumbDrop.designated + ' from ' + drag.source
                                    + ', keys: ' + drag.keys + ', accept: ' + drag.accepted)
                            }
                            // else if(drag.keys[0] === 'DraggingWindowAvatar'){
                            //    // console.log('------[DraggingWindowAvatar]: Enter ' + ' from ' + drag.source + ', keys: ' + drag.keys + ', accept: ' + drag.accepted)


                            //     //drag.accepted
                            // }
                        }

                        onExited: {
                            console.log("----------- workspaceThumb onExited")
                            if (drag.source.pendingDragRemove) {
                                hint.visible = false
                                drag.source.pendingDragRemove = hint.visible
                            }

                        }

                        onPositionChanged: {
                        //    console.log("----------- workspaceThumb onPositionChanged")
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
                            // if(drag.keys[0] === "DraggingWindowAvatar"){
                            //     //console.log("----------- DraggingWindowAvatar onPositionChanged"+view.indexAt(Qt.point(drag.source.x,drag.source.y)))
                            //    // console.log("----------- DraggingWindowAvatar onPositionChanged");
                            // }
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

                //center
                onCountChanged: {
                    view.width = manager.thumbSize.width * count;
                    view.x = (parent.width - view.width) / 2;
                    plusBtn.visible = count < 4;

					//default value 1
					windowThumbnail.model = $Model.windows(currentScreen, 1); 
                }


				Connections {
					target: $Model;
					onCurrentIndexChanged: {
						windowThumbnail.model = $Model.windows(currentScreen, currentIndex + 1);
					}
                    
				}
            }

            Button {
                id: plusBtn;
                text: "+";
                anchors.top: parent.top;
                anchors.right: parent.right;
				width: manager.thumbSize.width; 
				height: manager.thumbSize.height;
                onClicked: {
                    $Model.append();
                    consle.log(currentScreen);
                }
            }

			//window thumbnail
			Grid {
				x: 0;
				y: view.y + view.height;
                width: root.width;
                height: root.height - view.height;

				Repeater {
					id: windowThumbnail;
					PlasmaCore.WindowThumbnail {
						x: 0;
						y: 0;
						width: 400;
						height: 300;
						winId: modelData; 
				   }
				}
			}
		}
	}

	Component.onCompleted: {
		for (var i = 0; i < $Model.numScreens(); ++i) {
			var geom = $Model.screenGeometry(i);
			var src = 
				'import QtQuick 2.0;' +
				'Loader {' + 
				'	x: ' + geom.x + ';' + 
				'	property int screenWidth: ' + geom.width + ';' +
				'	height: 260;' +
				'	property int currentScreen: ' + i + ';' +
				'	sourceComponent: desktopThumbmailView;' + 
				'}';
			Qt.createQmlObject(src, root, "dynamicSnippet");
		}	
	}
}

