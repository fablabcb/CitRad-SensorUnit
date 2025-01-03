import QtQuick 2.15
import QtQuick.Controls 2.15 as Q

import citrad.style 1.0

Q.TabButton {
    id: root

    background: Rectangle {
        color: root.checked ? Style.primaryColor : "black"
        implicitHeight: Style.minTouchSize
    }
}
