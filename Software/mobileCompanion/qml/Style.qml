pragma Singleton

import QtQuick 2.15

QtObject {
    // fonts
    readonly property int fontSizeM: Math.floor(8 * fac)
    readonly property int fontSizeS: Math.floor(5 * fac)
    readonly property int fontSizeXS: Math.floor(4 * fac)

    readonly property real minTouchSize: fac * 30 // set to work with normalFontSize
    readonly property int baseRadius: 2
    readonly property int buttonMargins: fac * 5

    // basic colors
    readonly property color primaryColor: "#ffc107"
    readonly property color secondaryColor: "#0c4b4c"

    // color roles
    readonly property color actionColor: secondaryColor

    // colors for specific controls
    readonly property color buttonTextColor: "white"

    property int dpi: 110
    readonly property real fac: dpi / 110
}
