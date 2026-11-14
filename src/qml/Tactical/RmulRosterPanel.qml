// SPDX-License-Identifier: MIT
import QtQuick
import "components"

HudPanel {
    id: root
    property var robots: []
    property bool blue: false
    property bool enemy: false
    accent: blue ? "#39b8ff" : "#ff5366"
    title: (enemy ? "敌方阵容" : "我方阵容") + " · " + (blue ? "蓝方" : "红方")
    eyebrow: "1 / 3 / 7"
    showDiagonalTexture: false

    Column {
        x: 14; y: 46; width: parent.width - 28
        spacing: 14
        Repeater {
            model: root.robots
            Rectangle {
                id: card
                required property var modelData
                objectName: "tacticalRobot" + modelData.robotId
                readonly property bool known: modelData.health != null
                readonly property bool zero: known && Number(modelData.health) === 0
                width: parent.width
                height: (root.height - 104) / 3
                radius: 4; color: "#0d1a28"
                border.color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.4)
                Rectangle {
                    width: 3; height: parent.height; color: root.accent
                }
                Image {
                    x: 13; anchors.verticalCenter: parent.verticalCenter
                    width: 64; height: 64
                    source: "qrc:/images/top_robots/" + (root.blue ? "blue" : "red") + (card.modelData.icon === "guard" ? "_guard_avatar.png" : "_teammate_avatar_" + card.modelData.icon + ".png")
                    fillMode: Image.PreserveAspectFit
                }
                Text { x: 90; y: 18; text: card.modelData.slot + "  " + card.modelData.label; color: "#dfedfa"; font.pixelSize: 20; font.bold: true }
                Text {
                    objectName: "tacticalHealth" + card.modelData.robotId
                    x: 90; y: 49
                    text: card.known ? String(card.modelData.health) : "—"
                    color: card.zero ? "#ffcc7a" : root.accent
                    font.pixelSize: 34; font.bold: true
                }
                Text { anchors.right: parent.right; anchors.rightMargin: 16; y: 65; text: "HP"; color: "#7891a8"; font.pixelSize: 12 }
                Text { x: 90; anchors.bottom: parent.bottom; anchors.bottomMargin: 13; text: !card.known ? "等待血量" : (card.zero ? "血量已归零" : "已接收血量"); color: "#8199ae"; font.pixelSize: 12 }
                Rectangle {
                    anchors.fill: parent; color: "#ffac44"; visible: card.zero
                    SequentialAnimation on opacity {
                        running: card.zero && root.visible; loops: Animation.Infinite
                        NumberAnimation { from: 0.02; to: 0.12; duration: 800 }
                        NumberAnimation { from: 0.12; to: 0.02; duration: 800 }
                    }
                }
            }
        }
    }
    Text { x: 18; anchors.bottom: parent.bottom; anchors.bottomMargin: 12; text: "重装 · 步兵 · 哨兵"; color: "#6e899e"; font.pixelSize: 12 }
}
