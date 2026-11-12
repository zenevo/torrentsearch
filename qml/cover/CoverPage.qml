import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    Column {
        anchors.centerIn: parent
        spacing: Theme.paddingSmall
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Torrent")
            font.pixelSize: Theme.fontSizeMedium
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Search")
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.highlightColor
        }
    }
}
