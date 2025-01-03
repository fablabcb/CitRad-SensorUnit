import QtQuick 2.15

import citrad 1.0 as C


/**
  * The image roll will build a large image over time. On the right, data can be added and all the rest will shift to
  * the left ending up in a history-section.
  * In order to support mobile devices, the image is being tiled and then finally, once the max length is reached,
  * discarded. The far right side where updates can take place is dynamic.
*/
Item {
    id: root

    required property var provider

    implicitWidth: provider.sectionWidth * provider.sectionsCount
    height: provider ? provider.height : 0

    Row {
        id: row

        x: root.width - root.provider.activeImageWidth - row.width
        height: root.height
        spacing: 0

        Repeater {
            id: repeater

            model: root.provider.images

            delegate: C.ImageDisplay {
                width: root.provider.sectionWidth
                height: root.height
                image: modelData
            }
        }
    }

    Item {
        id: content

        x: root.width - provider.activeImageWidth
        width: root.sectionWidth
        height: root.height

        children: [root.provider]
    }
}
