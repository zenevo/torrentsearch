import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property var model: []
    // The ComboBox lives inside the list header, which is its own component
    // scope, so the selection is mirrored here.
    property string provider: "The Pirate Bay"
    readonly property bool canLoadMore: backend.supportsPaging(page.provider)
                                       && page.model.length > 0

    SilicaListView {
        id: listView
        model: page.model
        anchors.fill: parent

        header: Column {
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Torrent Search") }

            ComboBox {
                id: providerCombo
                label: qsTr("Source")
                currentIndex: 0
                menu: ContextMenu {
                    MenuItem { text: "The Pirate Bay"; onClicked: page.provider = text }
                    MenuItem { text: "Knaben"; onClicked: page.provider = text }
                    MenuItem { text: "TorrentsCSV"; onClicked: page.provider = text }
                    MenuItem { text: "SolidTorrents"; onClicked: page.provider = text }
                    MenuItem { text: "DMHY"; onClicked: page.provider = text }
                    MenuItem { text: "Rutor"; onClicked: page.provider = text }
                    MenuItem { text: "Nyaa"; onClicked: page.provider = text }
                }
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: qsTr("Search torrents…")
                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-search"
                EnterKey.onClicked: {
                    page.model = []
                    backend.search(text, page.provider)
                    focus = false
                }
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                size: BusyIndicatorSize.Small
                running: backend.busy
                visible: running
            }
        }

        footer: Item {
            width: listView.width
            height: page.canLoadMore && page.model.length > 0 ? Theme.itemSizeSmall : 0
            Button {
                anchors.centerIn: parent
                text: qsTr("Load more")
                enabled: !backend.busy
                visible: page.canLoadMore
                onClicked: backend.loadMore()
            }
        }

        delegate: ListItem {
            id: item
            width: listView.width
            contentHeight: Theme.itemSizeLarge

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                Label {
                    width: parent.width
                    text: modelData.name
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    text: qsTr("%1 · SE %2 / LE %3")
                          .arg(modelData.sizeText && modelData.sizeText.length > 0
                               ? modelData.sizeText : formatSize(modelData.size))
                          .arg(modelData.seeders)
                          .arg(modelData.leechers)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
            }

            onClicked: pageStack.push(Qt.resolvedUrl("TorrentPage.qml"), { torrent: modelData })
        }

        ViewPlaceholder {
            enabled: page.model.length === 0 && !backend.busy
            text: backend.error.length > 0 ? backend.error : qsTr("Search for a torrent")
            hintText: qsTr("Enter a query and press Enter")
        }
    }

    Connections {
        target: backend
        onResults: {
            if (append)
                page.model = page.model.concat(items)
            else
                page.model = items
        }
    }

    function formatSize(bytes) {
        if (!bytes || bytes <= 0)
            return "—"
        var units = [ "B", "KB", "MB", "GB", "TB" ]
        var i = 0
        var v = bytes
        while (v >= 1024 && i < units.length - 1) {
            v /= 1024
            i++
        }
        return v.toFixed(1) + " " + units[i]
    }
}
