import QtQuick 2.0
import QtGraphicalEffects 1.0

Image {
    id: img;

    property bool rounded: true
    property int imageRadius: 0
    property int desktopIndex: 1
    property string desktopScreenName: ""

    source: "image://BackgroundImageProvider/" + desktopIndex + "/" + desktopScreenName
    cache : false

    layer.enabled: true
    layer.effect: OpacityMask {
        maskSource: Rectangle {
            anchors.centerIn: parent
            width: img.width
            height: img.height
            radius: img.rounded ? imageRadius : 0
        }
    }

    function reloadImage() {
        img.sourceSize.width--
        img.sourceSize.width++
    }
}
