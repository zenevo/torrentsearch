import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property var torrent

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("Torrent") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: torrent.name
            }

            TextArea {
                id: magnetArea
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: 200
                text: torrent.magnet && torrent.magnet.length > 0
                      ? torrent.magnet
                      : backend.magnetFor(torrent.infoHash, torrent.name)
                readOnly: true
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            ButtonLayout {
                Button {
                    id: copyButton
                    text: qsTr("Copy magnet")
                    onClicked: {
                        Clipboard.text = magnetArea.text
                        text = qsTr("Copied!")
                        copyTimer.restart()
                    }
                }
                Button {
                    text: qsTr("Open")
                    onClicked: Qt.openUrlExternally(magnetArea.text)
                }
                Button {
                    id: saveButton
                    text: qsTr("Save .torrent")
                    onClicked: backend.downloadTorrent(torrent.infoHash, torrent.name,
                                                       torrent.torrentUrl ? torrent.torrentUrl : "")
                }
            }

            Label {
                id: statusLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                text: qsTr("SE %1 / LE %2").arg(torrent.seeders).arg(torrent.leechers)
            }
        }
    }

    Timer {
        id: copyTimer
        interval: 1500
        onTriggered: copyButton.text = qsTr("Copy magnet")
    }

    Connections {
        target: backend
        onTorrentSaved: statusLabel.text = qsTr("Saved: %1").arg(path)
        onTorrentFailed: statusLabel.text = qsTr("Failed: %1").arg(reason)
    }
}
