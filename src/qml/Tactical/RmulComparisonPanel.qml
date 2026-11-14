// SPDX-License-Identifier: MIT
import QtQuick
import "components"

HudPanel {
    id: root
    required property var ally
    required property var enemy
    property bool allyIsBlue: false
    title: "双方比赛数据"
    eyebrow: "3V3 SNAPSHOT"
    accent: "#46c9ec"
    readonly property color allyColor: allyIsBlue ? "#39b8ff" : "#ff5366"
    readonly property color enemyColor: allyIsBlue ? "#ff5366" : "#39b8ff"
    readonly property var metrics: [
        { key: "controlPoints", title: "控制度" },
        { key: "damage", title: "总攻击伤害" },
        { key: "health", title: "三车总剩余血量" }
    ]
    Row {
        x: 16; y: 46; width: parent.width - 32; spacing: 12
        Repeater {
            model: root.metrics
            Rectangle {
                id: metric
                required property var modelData
                readonly property var a: root.ally[modelData.key]
                readonly property var e: root.enemy[modelData.key]
                readonly property bool complete: a != null && e != null
                readonly property real ratio: complete && (Number(a) + Number(e)) > 0 ? Number(a) / (Number(a) + Number(e)) : 0.5
                width: (parent.width - 24) / 3
                height: root.height - 102
                color: "#091724"; border.color: "#193749"; radius: 4
                Text { x: 15; y: 16; text: metric.modelData.title; color: "#bed4e5"; font.pixelSize: 16 }
                Text { x: 15; y: 53; text: metric.a != null ? String(metric.a) : "—"; color: root.allyColor; font.pixelSize: 30; font.bold: true }
                Text { anchors.right: parent.right; anchors.rightMargin: 15; y: 97; text: metric.e != null ? String(metric.e) : "—"; color: root.enemyColor; font.pixelSize: 30; font.bold: true }
                Text { x: 15; y: 90; text: "我方"; color: "#738da3"; font.pixelSize: 12 }
                Text { anchors.right: parent.right; anchors.rightMargin: 15; y: 135; text: "敌方"; color: "#738da3"; font.pixelSize: 12 }
                Rectangle {
                    x: 15; anchors.bottom: parent.bottom; anchors.bottomMargin: 17; width: parent.width - 30; height: 5
                    color: metric.complete ? root.enemyColor : "#253744"
                    Rectangle {
                        height: parent.height; width: parent.width * metric.ratio
                        color: metric.complete ? root.allyColor : "#253744"
                        Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                    }
                }
            }
        }
    }
    Text {
        x: 18; anchors.bottom: parent.bottom; anchors.bottomMargin: 16
        text: "— 表示尚未收到 · 数据为最近接收快照 · 结果以裁判系统下发为准"
        color: "#7f99af"; font.pixelSize: 12
    }
}
