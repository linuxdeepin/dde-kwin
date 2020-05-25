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
					}
				}
			}
		}
	}

	Component {
        id: desktopThumbmailView;
        Rectangle {
            width: parent.width / 2;
            height: parent.height;
            ListView {
                id: view
                width: 0;
                height: parent.height;
                orientation: ListView.Horizontal;
                model: $Model

                delegate: Rectangle {
                    width: manager.thumbSize.width;
                    height: manager.thumbSize.height;
                    color: "gray";

                    DesktopThumbnail {
						id: desktopThumbnail;
						desktop: index + 1;
                        anchors.fill: parent;
                        MouseArea{
                            anchors.fill: parent;
							onClicked: {
								$Model.setCurrentIndex(index);
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
			var geom = $Model.geometry(i);
			var src = 
				'import QtQuick 2.0;' +
				'Loader {' + 
				'	x: ' + geom.x + ';' + 
				'	width: ' + geom.width + ';' +
				'	height: 260;' +
				'	property int currentScreen: ' + i + ';' +
				'	sourceComponent: desktopThumbmailView;' + 
				'}';
			Qt.createQmlObject(src, root, "dynamicSnippet");
		}	
	}
}

