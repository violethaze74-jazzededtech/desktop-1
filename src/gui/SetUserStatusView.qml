/*
 * Copyright (C) by Felix Weilbach <felix.weilbach@nextcloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

import QtQuick 2.6
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import com.nextcloud.desktopclient 1.0 as NC

ColumnLayout {
    id: rootLayout
    spacing: 0
    property NC.SetUserStatusDialogModel setUserStatusDialogModel

    Connections {
        target: setUserStatusDialogModel
        function onShowError() {
            errorDialog.open()
        }
    }

    FontMetrics {
        id: metrics
    }

    Item {
        Layout.margins: 8
    }

    Text {
        Layout.margins: 8
        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
        font.bold: true
        text: qsTr("Online status")
    }
        
    GridLayout {
        Layout.margins: 8
        Layout.alignment: Qt.AlignTop
        columns: 2
        rows: 2
        columnSpacing: 8
        rowSpacing: 8

        Button {
            Layout.fillWidth: true
            checked: NC.UserStatus.Online == setUserStatusDialogModel.onlineStatus
            checkable: true
            icon.source: setUserStatusDialogModel.onlineIcon
            icon.color: "transparent"
            text: qsTr("Online")
            onClicked: setUserStatusDialogModel.setOnlineStatus(NC.UserStatus.Online)
            implicitWidth: 100
        }
        Button {
            Layout.fillWidth: true
            checked: NC.UserStatus.Away == setUserStatusDialogModel.onlineStatus
            checkable: true
            icon.source: setUserStatusDialogModel.awayIcon
            icon.color: "transparent"
            text: qsTr("Away")
            onClicked: setUserStatusDialogModel.setOnlineStatus(NC.UserStatus.Away)
            implicitWidth: 100
            
        }
        Button {
            Layout.fillWidth: true
            checked: NC.UserStatus.DoNotDisturb == setUserStatusDialogModel.onlineStatus
            checkable: true
            icon.source: setUserStatusDialogModel.dndIcon
            icon.color: "transparent"
            text: qsTr("Do not disturb")
            onClicked: setUserStatusDialogModel.setOnlineStatus(NC.UserStatus.DoNotDisturb)
            implicitWidth: 100
        }
        Button {
            Layout.fillWidth: true
            checked: NC.UserStatus.Invisible == setUserStatusDialogModel.onlineStatus
            checkable: true
            icon.source: setUserStatusDialogModel.invisibleIcon
            icon.color: "transparent"
            text: qsTr("Invisible")
            onClicked: setUserStatusDialogModel.setOnlineStatus(NC.UserStatus.Invisible)
            implicitWidth: 100
        }
    }

    Item {
        Layout.margins: 8
    }

    Text {
        Layout.margins: 8
        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
        font.bold: true
        text: qsTr("Status message")
    }

    RowLayout {
        Layout.margins: 8
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true

        Button {
            Layout.preferredWidth: metrics.height * 2
            Layout.preferredHeight: metrics.height * 2
            text: setUserStatusDialogModel.userStatusEmoji
            onClicked: emojiDialog.open()
        }

        Popup {
            id: emojiDialog
            padding: 0
            margins: 0

            anchors.centerIn: Overlay.overlay
            
            EmojiPicker {
                id: emojiPicker

                onChosen: {
                    setUserStatusDialogModel.userStatusEmoji = emoji
                    emojiDialog.close()
                }
            }
        }

        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("What is your Status?")
            text: setUserStatusDialogModel.userStatusMessage
            onEditingFinished: setUserStatusDialogModel.setUserStatusMessage(text)
        }
    }

    ColumnLayout {
        Layout.margins: 8
        Layout.alignment: Qt.AlignTop

        Repeater {
            model: setUserStatusDialogModel.predefinedStatusesCount

            Button {
                id: control
                Layout.fillWidth: true
                flat: !hovered
                hoverEnabled: true
                text: setUserStatusDialogModel.predefinedStatus(index).icon + " <b>" + setUserStatusDialogModel.predefinedStatus(index).message + "</b> - " + setUserStatusDialogModel.predefinedStatusClearAt(index)
                onClicked: setUserStatusDialogModel.setPredefinedStatus(index)
            }
        }
    }

    Item {
        Layout.margins: 8
    }

   RowLayout {
       Layout.margins: 8
       Layout.alignment: Qt.AlignTop

       Text {
           text: qsTr("Clear status message after")
       }

       ComboBox {
           Layout.fillWidth: true
           model: setUserStatusDialogModel.clearAtStages
           displayText: setUserStatusDialogModel.clearAt
           onActivated: setUserStatusDialogModel.setClearAt(index)
       }
   }

    RowLayout {
        Layout.margins: 8
        Layout.alignment: Qt.AlignTop
        
        Button {
            Layout.fillWidth: true
            text: qsTr("Clear status message")
            onClicked: setUserStatusDialogModel.clearUserStatus()
        }
        Button {
            highlighted: true
            Layout.fillWidth: true
            text: qsTr("Set status message")
            onClicked: setUserStatusDialogModel.setUserStatus()
        }
    }

    MessageDialog {
        id: errorDialog
        icon: StandardIcon.Critical
        title: qsTr("Set user status")
        text: setUserStatusDialogModel.errorMessage
        visible: false
    }
}
