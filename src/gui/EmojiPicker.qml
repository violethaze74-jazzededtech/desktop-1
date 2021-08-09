import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import com.nextcloud.desktopclient 1.0 as NC

ColumnLayout {
    readonly property var emojiModel: NC.EmojiModel {}

    signal chosen(string emoji)

    spacing: 0

    FontMetrics {
        id: metrics
    }

    ListView {
        id: headerLayout
        Layout.fillWidth: true
        implicitWidth: contentItem.childrenRect.width
        implicitHeight: contentItem.childrenRect.height

        boundsBehavior: Flickable.DragOverBounds
        clip: true
        orientation: ListView.Horizontal

        model: emojiModel.emojiCategoriesModel

        delegate: ItemDelegate {
            width: metrics.height * 2
            height: metrics.height * 2

            Text {
                anchors.centerIn: parent
                text: emoji
            }

            Rectangle {
                anchors.bottom: parent.bottom

                width: parent.width
                height: 2

                visible: category === emojiCategory

                color: "grey"
            }

            onClicked: {
                emojiModel.setCategory(label)
            }
        }

    }

    Rectangle {
        height: 1
        Layout.fillWidth: true
        color: "grey"
    }

    GridView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: metrics.height * 8

        cellWidth: metrics.height * 2
        cellHeight: metrics.height * 2

        boundsBehavior: Flickable.DragOverBounds
        clip: true

        model: emojiModel.model

        delegate: ItemDelegate {

            width: metrics.height * 2
            height: metrics.height * 2

            Text {
                anchors.centerIn: parent
                text: modelData === undefined ? "" : modelData.unicode
            }

            onClicked: {
                chosen(modelData.unicode);
                emojiModel.emojiUsed(modelData);
            }
        }

        ScrollBar.vertical: ScrollBar {}
        
    }

}
