import QtQuick 2.15
import QtQuick.Controls 2.15 as Q

import citrad.style 1.0

Q.Button {
    id: root

    contentItem: Text {
        id: text
        text: root.text
        font: root.font
        opacity: enabled ? 1.0 : 0.3
        color: Style.buttonTextColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: root.down ? Qt.lighter(Style.actionColor) : Style.actionColor
        implicitHeight: Style.minTouchSize
        implicitWidth: Math.max(2 * Style.minTouchSize, text.contentHeight + 2 * Style.buttonMargins)
        radius: Style.baseRadius
    }
}
