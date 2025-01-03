import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import citrad.style 1.0
import "components"

RowLayout {
    id: root

    property bool isConnected: false
    property alias selectionEnabled: selector.enabled
    property var model: []
    property alias currentIndex: selector.currentIndex
    property bool allowFileSelection: false

    signal requestConnect(var text)
    signal requestDisconnect
    signal requestOpenFile

    implicitHeight: selector.height
    spacing: 0

    ComboBox {
        id: selector

        Layout.fillHeight: true
        Layout.fillWidth: true

        model: root.allowFileSelection ? root.model.concat("Select File...") : root.model
        enabled: count > 0

        onActivated: if (root.allowFileSelection && currentIndex === count - 1)
                         root.requestOpenFile()
    }

    Button {
        id: connectBtn

        Layout.fillHeight: true

        enabled: selector.count > 0
        visible: !(root.allowFileSelection && root.currentIndex === selector.count - 1)
        text: root.isConnected ? "Disconnect" : "Connect"
        onClicked: root.isConnected ? root.requestDisconnect() : root.requestConnect(selector.currentText)

        font.pointSize: Style.fontSizeM
    }
}
