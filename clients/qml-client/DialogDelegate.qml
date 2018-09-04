import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.0

ItemDelegate {
    id: dialogDelegate
    height: picture.height + topPadding + bottomPadding
    width: 200
    property int margin: (pictureFrame.width - picture.width) / 2
    readonly property int defaultMargin: dialogDelegate.margin

    property string displayName
    property int unreadMessageCount
    property var timestamp

    property var lastMessage

    property bool debugGeometry: false
    property int smallSpacing: spacing / 2

    contentItem: Item {
        id: content
        Rectangle {
            id: pictureFrame
            height: picture.height
            width: height
            anchors.verticalCenter: parent.verticalCenter
            border.color: "pink"
            border.width: 1
            visible: content.width > width * 4

            Rectangle {
                id: picture
                readonly property var colors: [
                    Material.Purple,
                    Material.DeepPurple,
                    Material.Blue,
                    Material.LightBlue,
                    Material.Cyan,
                    Material.Teal,
                    Material.LightGreen,
                    Material.Lime,
                    Material.Amber,
                    Material.Orange,
                    Material.DeepOrange,
                    Material.Brown,
                    Material.Grey,
                    Material.BlueGrey,
                ]
                function getColor(username) {
                    return colors[Qt.md5(username).charCodeAt(0) % colors.length]
                }

                color: Material.color(getColor(dialogDelegate.displayName))
                width: height
                height: 42
                radius: defaultMargin
                border.color: "black"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    font.pixelSize: parent.width - defaultMargin
                    text: displayName ? displayName[0] : "?"
                    font.capitalization: Font.AllUppercase
                }
            }
        }

        Item {
            id: contentColumn
            anchors.left: pictureFrame.visible ? pictureFrame.right : content.left
            anchors.leftMargin: pictureFrame.visible ? dialogDelegate.spacing : 0
            anchors.right: content.right
            anchors.top: content.top
            anchors.bottom: content.bottom

            Item {
                anchors.top: parent.top
                width: parent.width
                height: displayNameLabel.implicitHeight
                Rectangle {
                    id: chatTypeIcon
                    radius: 4
                    width: height
                    height: displayNameLabel.height
                    visible: model.dialogType === 1
                    color: "blue"
                    anchors.verticalCenter: parent.verticalCenter
                }

                InlineHeader {
                    id: displayNameLabel
                    anchors.left: chatTypeIcon.visible ? chatTypeIcon.right : parent.left
                    anchors.leftMargin: chatTypeIcon.visible ? dialogDelegate.smallSpacing : 0
                    anchors.right: deliveryIcon.visible ? deliveryIcon.left : lastMessageDateTime.left
                    anchors.rightMargin: dialogDelegate.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    text: dialogDelegate.displayName
                }

                Rectangle {
                    id: deliveryIcon
                    width: height
                    height: lastMessageDateTime.font.pixelSize
                    color: "green"
                    visible: !model.draft && (typeof(model.lastMessage) != "undefined" && model.lastMessage.flags & 1)
                    anchors.right: lastMessageDateTime.left
                    anchors.rightMargin: dialogDelegate.smallSpacing
                    anchors.verticalCenter: parent.verticalCenter
                }

                Label {
                    id: lastMessageDateTime
                    color: "gray"
                    horizontalAlignment: Qt.AlignRight
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 14

                    function formatRelativeCurrentDay(date)
                    {
                        var today = new Date
                        today.setHours(0,0,0,0)
                        if (dialogDelegate.timestamp >= today) {
                            return Qt.formatTime(dialogDelegate.timestamp, "hh:mm")
                        }
                        today.setDate(today.getDate() - 1)
                        console.log(today)
                        if (dialogDelegate.timestamp >= today) {
                            return qsTr("Yesterday")
                        }
                        today.setDate(today.getDate() - 6)
                        if (dialogDelegate.timestamp >= today) {
                            return dialogDelegate.timestamp.toLocaleString(Qt.locale(), "ddd")
                        }
                        return Qt.formatDateTime(dialogDelegate.timestamp, "d MMM hh:mm")
                    }

                    text: typeof(dialogDelegate.timestamp) === "undefined" ? "" : formatRelativeCurrentDay(dialogDelegate.timestamp)
                }
                Rectangle {
                    opacity: 0.1
                    color: "blue"
                    anchors.fill: parent
                    visible: dialogDelegate.debugGeometry
                }
            }

            Item {
                anchors.bottom: parent.bottom
                width: contentColumn.width
                height: messagePreviewLabel.implicitHeight

                Label {
                    id: messagePreviewLabel
                    elide: Text.ElideRight
                    text: prefixStyledText + contentText
                    anchors.left: parent.left
                    anchors.rightMargin: unreadIndicator.visible ? dialogDelegate.spacing : 0
                    anchors.right: unreadIndicator.visible ? unreadIndicator.left : parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    readonly property string prefixStyledText: prefixText ? "<font color=\"" + prefixColor + "\">" + prefixText + "</font>" : ""
                    readonly property color prefixColor: model.draft ? "red" : palette.link
                    readonly property string prefixText: {
                        if (model.draft) {
                            return "Draft: "
                        }
                        if (typeof(model.activeMessageActions) != "undefined") {
                            return ""
                        }
                        if (lastMessage) {
                            if (lastMessage.flags & 1) {
                                return "You: "
                            }
                            if (model.dialogType === 1) {
                                return lastMessage.senderFirstName + ": "
                            }
                        }
                        return ""
                    }

                    readonly property string contentText: {
                        if (model.draft)
                            return model.draft

                        var emphasedText
                        if (typeof(model.activeMessageActions) != "undefined" && model.activeMessageActions.count > 0) {
                            var firstAction = model.activeMessageActions.get(0)
                            if (firstAction.type === "typing") {
                                emphasedText = "..." + firstAction.contact + " is typing"
                            }
                            if (emphasedText) {
                                return "<font color=\"" + emphasedContentColor + "\">" + emphasedText + "</font>"
                            }
                            // Ignore unknown actions
                        }
                        if (typeof(lastMessage) === "undefined") {
                            return ""
                        }
                        if (lastMessage.type === "text") {
                            return lastMessage.text
                        }
                        if (lastMessage.type === "photo") {
                            emphasedText = "Photo"
                        } else if (lastMessage.type === "video") {
                            emphasedText = "Video"
                        } else {
                            // Do *not* ignore unknown messages
                            emphasedText = "Unsupported content"
                        }
                        return "<font color=\"" + emphasedContentColor + "\">" + emphasedText + "</font>"
                    }
                    readonly property color emphasedContentColor: palette.link
                }
                UnreadMessageIndicator {
                    id: unreadIndicator
                    count: dialogDelegate.unreadMessageCount
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle {
                    opacity: 0.1
                    color: "red"
                    anchors.fill: parent
                    visible: dialogDelegate.debugGeometry
                }
            }
        }
    }
}
