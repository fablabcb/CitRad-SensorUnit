import QtQuick 2.15
import QtQuick.Controls 2.15

import "components"

Item {
    id: root

    implicitHeight: selector.height

    property alias commands: selector.model

    signal sendCommandRequested(string command, string text)

    ComboBox {
        id: selector
        anchors {
            left: parent.left
            right: valueEdit.left
        }
    }

    TextField {
        id: valueEdit
        anchors {
            left: selector.right
            right: sendButton.left
        }
        width: 300
    }

    Button {
        id: sendButton
        anchors {
            right: parent.right
        }
        text: "Send"
        width: 300
        height: selector.height

        onClicked: root.sendCommandRequested(selector.currentText, valueEdit.text)
    }
}
