import QtQuick 2.15
import QtQml.Models 2.15

import citrad 1.0

Item {
    id: root

    property ResultModel resultModel
    property real speedToBinFactor: 1
    property int activeSampleIdx: 0
    readonly property int minSampleIndex: activeSampleIdx - width

    onMinSampleIndexChanged: singleGroup.removeOld()

    DelegateModel {
        id: visualModel
        model: root.resultModel

        groups: [
            DelegateModelGroup {
                id: singleGroup
                name: "all"
                includeByDefault: true

                function removeOld() {
                    if (count === 0)
                        return

                    if (get(0).model.sampleIndex < minSampleIndex) {
                        remove(0, 1)
                        removeOld()
                    }
                }
            }
        ]

        filterOnGroup: "all"

        delegate: Item {
            required property var model

            x: {
                const baseX = root.width - (root.activeSampleIdx - model.sampleIndex)
                return baseX - model.detectionAge - (model.isForward ? 0 : width)
            }

            width: model.speedsCount
            height: root.height

            Rectangle {
                id: indicator
                property int yOffset: model.medianSpeed * root.speedToBinFactor
                y: root.height / 2 + yOffset * (model.isForward ? -1 : 1)
                height: 1
                width: parent.width
                color: "white"

                Rectangle {
                    y: model.isForward ? -40 : 20
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: text.contentHeight
                    width: text.contentWidth + 6
                    radius: Style.baseRadius
                    color: Qt.rgba(1, 1, 1, 0.8)
                    Text {
                        id: text
                        anchors.centerIn: parent
                        text: model.medianSpeed.toFixed(1)
                    }
                }
            }

            Rectangle {
                x: 0
                y: model.isForward ? 0 : parent.height / 2
                width: 1
                height: parent.height / 2
                color: "grey"
            }
            Rectangle {
                x: parent.width - 1
                y: model.isForward ? 0 : parent.height / 2
                width: 1
                height: parent.height / 2
                color: "grey"
            }
        }
    }

    Repeater {
        model: visualModel
    }
}
