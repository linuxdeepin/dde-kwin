import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kwin 2.0 as KWin

// https://techbase.kde.org/Development/Tutorials/KWin/WindowSwitcher
KWin.Switcher {
    id: tabBox
    currentIndex: thumbnailGridView.currentIndex

    PlasmaCore.Dialog {
        id: dialog
        location: PlasmaCore.Types.Floating
        visible: tabBox.visible
        flags: Qt.X11BypassWindowManagerHint
        x: tabBox.screenGeometry.x + tabBox.screenGeometry.width * 0.5 - dialogMainItem.width * 0.5
        y: tabBox.screenGeometry.y + tabBox.screenGeometry.height * 0.5 - dialogMainItem.height * 0.5

        onVisibleChanged: {
            if (visible) {
                dialogMainItem.calculateColumnCount();
            } else {
                thumbnailGridView.highCount = 0;
            }
        }

        mainItem: Item {
            id: dialogMainItem

            property int maxWidth: tabBox.screenGeometry.width * 0.9
            property int maxHeight: tabBox.screenGeometry.height * 0.7
            property real screenFactor: tabBox.screenGeometry.width / tabBox.screenGeometry.height
            property int maxGridColumnsByWidth: Math.floor(maxWidth / thumbnailGridView.cellWidth)

            property int gridColumns: maxGridColumnsByWidth
            property int gridRows: Math.ceil(thumbnailGridView.count / gridColumns)
            property int optimalWidth: thumbnailGridView.cellWidth * gridColumns
            property int optimalHeight: thumbnailGridView.cellHeight * gridRows
            property bool canStretchX: false
            property bool canStretchY: false
            width: Math.min(Math.max(thumbnailGridView.cellWidth, optimalWidth), maxWidth)
            height: Math.min(Math.max(thumbnailGridView.cellHeight, optimalHeight), maxHeight)

            clip: true

            // simple greedy algorithm
            function calculateColumnCount() {
                // respect screenGeometry
                var c = Math.min(thumbnailGridView.count, maxGridColumnsByWidth);

                var residue = thumbnailGridView.count % c;
                if (residue == 0) {
                    gridColumns = c;
                    return;
                }

                // start greedy recursion
                gridColumns = columnCountRecursion(c, c, c - residue);
            }

            // step for greedy algorithm
            function columnCountRecursion(prevC, prevBestC, prevDiff) {
                var c = prevC - 1;

                // don't increase vertical extent more than horizontal
                // and don't exceed maxHeight
                if (prevC * prevC <= thumbnailGridView.count + prevDiff ||
                        maxHeight < Math.ceil(thumbnailGridView.count / c) * thumbnailGridView.cellHeight) {
                    return prevBestC;
                }
                var residue = thumbnailGridView.count % c;
                // halts algorithm at some point
                if (residue == 0) {
                    return c;
                }
                // empty slots
                var diff = c - residue;

                // compare it to previous count of empty slots
                if (diff < prevDiff) {
                    return columnCountRecursion(c, c, diff);
                } else if (diff == prevDiff) {
                    // when it's the same try again, we'll stop early enough thanks to the landscape mode condition
                    return columnCountRecursion(c, prevBestC, diff);
                }
                // when we've found a local minimum choose this one (greedy)
                return columnCountRecursion(c, prevBestC, diff);
            }

            property bool mouseEnabled: false
            MouseArea {
                id: mouseDetector
                anchors.fill: parent
                hoverEnabled: true
                onPositionChanged: dialogMainItem.mouseEnabled = true
            }

            // just to get the margin sizes
            PlasmaCore.FrameSvgItem {
                id: hoverItem
                imagePath: "widgets/viewitem"
                prefix: "hover"
                visible: false
            }

            GridView {
                id: thumbnailGridView
                model: tabBox.model
                // interactive: false // Disable drag to scroll

                anchors.fill: parent

                property int captionRowHeight: 22
                property int thumbnailWidth: 300
                property int thumbnailHeight: thumbnailWidth * (1.0/dialogMainItem.screenFactor)
                cellWidth: hoverItem.margins.left + thumbnailWidth + hoverItem.margins.right
                cellHeight: hoverItem.margins.top + captionRowHeight + thumbnailHeight + hoverItem.margins.bottom
                height: cellHeight

                // allow expansion on increasing count
                property int highCount: 0
                onCountChanged: {
                    if (highCount < count) {
                        dialogMainItem.calculateColumnCount();
                        highCount = count;
                    }
                }

                delegate: Item {
                    width: thumbnailGridView.cellWidth
                    height: thumbnailGridView.cellHeight

                    MouseArea {
                        anchors.fill: parent
                        // hoverEnabled: dialogMainItem.mouseEnabled
                        // onEntered: parent.hover()
                        onClicked: {
                            parent.select()
                            // dialog.close() // Doesn't end the effects until you release Alt.
                        }
                    }
                    function select() {
                        thumbnailGridView.currentIndex = index;
                        thumbnailGridView.currentIndexChanged(thumbnailGridView.currentIndex);
                    }

                    Item {
                        z: 0
                        anchors.fill: parent
                        anchors.leftMargin: hoverItem.margins.left
                        anchors.topMargin: hoverItem.margins.top
                        anchors.rightMargin: hoverItem.margins.right
                        anchors.bottomMargin: hoverItem.margins.bottom

                        RowLayout {
                            id: captionRow
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.right: parent.right
                            height: thumbnailGridView.captionRowHeight
                            spacing: 4

                            QIconItem {
                                id: iconItem
                                // source: model.icon
                                icon: model.icon
                                width: parent.height
                                height: parent.height
                                state: index == thumbnailGridView.currentIndex ? QIconItem.ActiveState : QIconItem.DefaultState
                            }

                            PlasmaComponents.Label {
                                text: model.caption
                                height: parent.height
                                // width: parent.width - captionRow.spacing - iconItem.width
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            PlasmaComponents.ToolButton {
                                visible: model.closeable && typeof tabBox.model.close !== 'undefined' || false
                                iconSource: 'window-close-symbolic'
                                onClicked: {
                                    tabBox.model.close(index)
                                }
                            }
                        }

                        // Cannot draw icon on top of thumbnail.
                        KWin.ThumbnailItem {
                            wId: windowId
                            // clip: true
                            // clipTo: thumbnailGridView
                            clip: true
                            clipTo: parent
                            anchors.fill: parent
                            anchors.topMargin: captionRow.height
                        }
                    }
                } // GridView.delegate

                highlight: PlasmaCore.FrameSvgItem {
                    id: highlightItem
                    imagePath: "widgets/viewitem"
                    prefix: "hover"
                    anchors.fill: thumbnailGridView.currentItem
                }

                // property int selectedIndex: -1
                Connections {
                    target: tabBox
                    onCurrentIndexChanged: {
                        thumbnailGridView.currentIndex = tabBox.currentIndex
                    }
                }

                // keyNavigationEnabled: true // Requires: Qt 5.7 and QtQuick 2.? (2.7 didn't work).
                // keyNavigationWraps: true // Requires: Qt 5.7 and QtQuick 2.? (2.7 didn't work).

            } // GridView

            
            // This doesn't work, nor does keyboard input work on any other tabbox skin (KDE 5.7.4)
            // It does work in the preview however.
            Keys.onPressed: {
                console.log('keyPressed', event.key)
                if (event.key == Qt.Key_Left) {
                    thumbnailGridView.moveCurrentIndexLeft();
                } else if (event.key == Qt.Key_Right) {
                    thumbnailGridView.moveCurrentIndexRight();
                } else if (event.key == Qt.Key_Up) {
                    thumbnailGridView.moveCurrentIndexUp();
                } else if (event.key == Qt.Key_Down) {
                    thumbnailGridView.moveCurrentIndexDown();
                } else {
                    return;
                }

                thumbnailGridView.currentIndexChanged(thumbnailGridView.currentIndex);
            }
        } // Dialog.mainItem
    } // Dialog
}
