import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.15

import citrad 1.0
import "components"

ApplicationWindow {
    id: root
    height: 1000
    width: 1000
    visible: true

    property Item superRoot: spectrumTab
    property QmlApi api
    property int historySectionWidth: 512
    property int historySectionCount: 5

    title: qsTr("CitRad Companion")

    StackLayout {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: bar.top
        }

        currentIndex: bar.currentIndex
        Loader {
            id: spectrumTab
            active: root.api
            sourceComponent: Rectangle {
                color: "black"

                ColumnLayout {
                    spacing: 0
                    anchors.fill: parent

                    DeviceSelector {
                        id: spectrumHeader

                        Layout.fillWidth: true
                        Layout.minimumHeight: implicitHeight
                        Layout.maximumHeight: implicitHeight

                        allowFileSelection: api.isMobile === false
                        isConnected: api.devices.isRawPortConnected
                        model: api.devices.allDevices
                        selectionEnabled: api.devices.isRawPortConnected === false

                        onRequestConnect: api.devices.connectRawPort(text)
                        onRequestDisconnect: api.devices.disconnectRawPort()
                        onRequestOpenFile: fileDialog.visible = true
                    }

                    Item {
                        Layout.preferredHeight: 0
                        Layout.fillHeight: true
                    }

                    MouseArea {
                        id: finder

                        Layout.fillWidth: true
                        Layout.preferredHeight: (pressed ? 3 : 1) * Style.minTouchSize
                        Layout.maximumHeight: 2 * Style.minTouchSize
                        z: 1

                        function toFinderX(spectrumX) {
                            return (spectrumX / spectrumRoll.width * finder.width)
                        }

                        ImageDisplay {
                            id: finderImage
                            anchors.fill: parent
                            image: spectrumVisualizer.overviewImage
                            endSeam: spectrumVisualizer.overviewImageSeam
                        }
                        Rectangle {
                            id: viewMarker
                            x: spectrumFlickable.contentX * finder.width / spectrumRoll.width
                            height: finderImage.height
                            width: spectrumFlickable.width * finder.width / spectrumRoll.width
                            color: "transparent"
                            border {
                                color: "white"
                                width: 1
                            }
                        }

                        onPressed: mouse => {
                                       internal.followData = false
                                       spectrumFlickable.contentX = spectrumRoll.width * (mouse.x - viewMarker.width / 2) / finder.width
                                   }
                        onPositionChanged: mouse => {
                                               if (pressed)
                                               spectrumFlickable.contentX = spectrumRoll.width
                                               * (mouse.x - viewMarker.width / 2) / finder.width
                                           }
                    }

                    Item {
                        Layout.preferredHeight: 0
                        Layout.fillHeight: true
                    }

                    Flickable {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 500
                        Layout.preferredHeight: api.spectrum.sampleSize
                        Layout.maximumHeight: api.spectrum.sampleSize

                        id: spectrumFlickable
                        visible: true

                        clip: true
                        boundsMovement: Flickable.StopAtBounds
                        boundsBehavior: Flickable.StopAtBounds

                        contentHeight: spectrumRoll.height
                        contentWidth: spectrumRoll.width

                        onFlickStarted: internal.followData = false
                        onDraggingChanged: internal.followData = false

                        function follow() {
                            spectrumFlickable.contentX = spectrumFlickable.contentWidth
                        }

                        ImageRoll {
                            id: spectrumRoll

                            provider: SpectrumVisualizer {
                                id: spectrumVisualizer

                                sectionWidth: root.historySectionWidth
                                sectionsCount: root.historySectionCount

                                spectrumController: api.spectrum
                                onTextureUpdated: {
                                    finderImage.update()
                                    if (internal.followData)
                                        spectrumFlickable.follow()
                                    else
                                        spectrumFlickable.contentX -= 1
                                }
                            }
                        }
                        Rectangle {
                            id: spectrumDimmer

                            property bool active: true

                            color: Qt.rgba(0, 0, 0, 0.6)
                            anchors.fill: spectrumRoll
                            visible: active
                        }

                        ImageRoll {
                            id: resultRoll

                            provider: ResultVisualizer {
                                sectionWidth: root.historySectionWidth
                                sectionsCount: root.historySectionCount

                                spectrumController: api.spectrum
                                resultModel: api.results
                                speedToBinFactor: api.spectrum.speedToBinFactor
                            }
                        }
                        DetectionVisualizer {
                            anchors.fill: spectrumRoll
                            resultModel: api.results
                            speedToBinFactor: api.spectrum.speedToBinFactor
                            activeSampleIdx: api.spectrum.activeSampleIdx
                        }
                    }

                    Item {
                        Layout.preferredHeight: 0
                        Layout.fillHeight: true
                    }
                }

                ColumnLayout {
                    anchors {
                        right: parent.right
                        rightMargin: spacing
                        bottom: parent.bottom
                        bottomMargin: 50
                    }
                    width: (api.isMobile ? 100 : 150) * Style.fac

                    Item {
                        Layout.fillHeight: true
                    }
                    Button {
                        text: "Toggle Contrast"
                        visible: internal.fullUi
                        onClicked: spectrumDimmer.active = !spectrumDimmer.active
                        Layout.fillWidth: true
                    }
                    Button {
                        visible: internal.fullUi
                        text: internal.followData ? "Disable Autoscroll" : "Enable Autoscroll"
                        onClicked: {
                            internal.followData = !internal.followData
                            if (internal.followData)
                                spectrumFlickable.follow()
                        }
                        Layout.fillWidth: true
                    }
                    Button {
                        text: "Get Next 30"
                        visible: api.devices.isInFileMode && internal.fullUi
                        onClicked: api.devices.readNextSamplesFromFile(30)
                        Layout.fillWidth: true
                    }
                    Button {
                        text: "Get Next 500"
                        visible: api.devices.isInFileMode && internal.fullUi
                        onClicked: api.devices.readNextSamplesFromFile(500)
                        Layout.fillWidth: true
                    }
                    Button {
                        text: "Load File"
                        visible: api.isMobile === false && internal.fullUi

                        onClicked: {
                            api.devices.openRawFile("data.bin")
                            api.devices.readNextSamplesFromFile(100)
                        }
                        Layout.fillWidth: false
                        Layout.alignment: Qt.AlignRight
                    }
                    Button {
                        text: "H"
                        onClicked: internal.fullUi = !internal.fullUi
                        Layout.fillWidth: false
                        Layout.maximumWidth: 40 * Style.fac
                        Layout.alignment: Qt.AlignRight
                    }
                }
            }
        }
        Loader {
            id: textTab
            active: root.api
            sourceComponent: Item {
                DeviceSelector {
                    id: logHeader

                    width: parent.width

                    isConnected: api.devices.isTextPortConnected
                    model: api.devices.allDevices
                    selectionEnabled: api.devices.isTextPortConnected === false

                    onRequestConnect: api.devices.connectTextPort(text)
                    onRequestDisconnect: api.devices.disconnectTextPort()
                }

                ListView {
                    model: api.textLog
                    anchors {
                        top: logHeader.bottom
                        left: parent.left
                        right: parent.right
                        bottom: commandSelector.top
                        bottomMargin: 20
                    }

                    clip: true
                    boundsMovement: Flickable.StopAtBounds
                    boundsBehavior: Flickable.StopAtBounds
                    onCountChanged: positionViewAtEnd()

                    delegate: Item {
                        height: text.contentHeight
                        Text {
                            id: timeText
                            text: model.time
                            font: text.font
                        }
                        Text {
                            id: text
                            anchors {
                                left: timeText.right
                                leftMargin: 10
                            }
                            text: {
                                let prefix = ["In   ", "Out  ", "Stat "]
                                return prefix[model.type] + model.text
                            }
                            color: {
                                let colors = ["blue", "orange", "grey"]
                                return colors[model.type]
                            }
                            font.family: "Monospace"
                            font.pointSize: api.isMobile ? Style.fontSizeXS : Style.fontSizeM
                        }
                    }
                }
                CommandSelector {
                    id: commandSelector
                    visible: false // not implemented
                    enabled: api.devices.isTextPortConnected
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }

                    commands: ["test 1", "test 2"]
                    onSendCommandRequested: api.devices.sendCommand(command, text)
                }
            }
        }
        ColumnLayout {
            id: detectionsTab

            Row {
                id: header

                Layout.margins: 10
                Layout.preferredHeight: 20
                Layout.fillWidth: true
                spacing: 10 * Style.fac

                Text {
                    id: idxHeader
                    width: contentWidth
                    text: "Idx"
                }
                Text {
                    id: directionHeader
                    width: contentWidth
                    text: "Direction"
                }
                Text {
                    id: speedHeader
                    width: 100 * Style.fac
                    text: "Speed"
                }
            }

            ListView {
                Layout.margins: 10
                Layout.fillHeight: true
                Layout.fillWidth: true

                model: api ? api.results : []
                delegate: Row {
                    id: delegate

                    spacing: header.spacing

                    Text {
                        width: idxHeader.width
                        text: model.index
                    }
                    Text {
                        width: directionHeader.width
                        text: model.isForward ? "Forward" : "Reverse"
                    }
                    Text {
                        width: speedHeader.width
                        text: model.medianSpeed.toFixed(0) + " km/h"
                    }
                }
            }
        }
    }

    TabBar {
        id: bar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        currentIndex: 0
        spacing: 0

        TabButton {
            text: "Spectrum"
        }
        TabButton {
            text: "Text-Log"
        }
        TabButton {
            text: "Detections"
        }
    }

    Component.onCompleted: {
        const dpi = screen.pixelDensity * 25.4
        Style.dpi = dpi
    }

    FileDialog {
        id: fileDialog

        title: "Please choose a file"
        folder: shortcuts.home
        onAccepted: api.devices.openRawFile(fileDialog.fileUrls[0])
    }

    Shortcut {
        sequence: "Esc"
        onActivated: root.close()
    }

    QtObject {
        id: internal

        property bool followData: true
        property bool fullUi: true
    }
}
