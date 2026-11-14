// SPDX-License-Identifier: MIT
import QtQuick
import "components"

Item {
    id: root
    required property var gameDataContext
    required property var ally
    required property var enemy
    property bool allyIsBlue: false
    readonly property bool hasMatch: gameDataContext !== null && Boolean(gameDataContext.hasMatchStatus)
    readonly property string allyControlText: ally.controlPoints != null ? String(ally.controlPoints) : "—"
    readonly property string enemyControlText: enemy.controlPoints != null ? String(enemy.controlPoints) : "—"
    readonly property string clockText: {
        if (!hasMatch) return "--:--"
        var secs = Math.max(0, Math.floor(Number(gameDataContext.remainingTime)))
        return String(Math.floor(secs / 60)).padStart(2, "0") + ":" + String(secs % 60).padStart(2, "0")
    }

    Repeater {
        model: 2
        HudPanel {
            id: wing
            required property int index
            readonly property bool blue: index === 0 ? root.allyIsBlue : !root.allyIsBlue
            readonly property var team: index === 0 ? root.ally : root.enemy
            x: index === 0 ? 0 : root.width - width
            width: (root.width - 400) / 2
            height: root.height
            title: (index === 0 ? "我方 · " : "敌方 · ") + (blue ? "蓝方" : "红方")
            accent: blue ? "#39b8ff" : "#ff5366"
            eyebrow: "CONTROL POINTS"
            Text {
                x: 28; y: 54
                text: "控制度"
                color: "#c6d8e8"; font.pixelSize: 19
            }
            Text {
                objectName: wing.index === 0 ? "tacticalAllyControlPoints" : "tacticalEnemyControlPoints"
                anchors.right: parent.right; anchors.rightMargin: 28
                y: 34
                text: wing.index === 0 ? root.allyControlText : root.enemyControlText
                color: wing.accent; font.pixelSize: 48; font.bold: true
            }
            Rectangle {
                x: 28; y: 102; width: parent.width - 56; height: 4
                color: "#1b2b3a"
                Rectangle {
                    height: parent.height
                    width: wing.team.controlPoints != null ? parent.width * Math.min(1, Number(wing.team.controlPoints) / 600) : 0
                    color: wing.accent
                    Behavior on width { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
                }
            }
            Text {
                x: 28; y: 115
                text: wing.team.controlPoints != null ? "控制度越高越有利" : "等待控制度数据"
                font.pixelSize: 12; color: "#8fa8ba"
            }
        }
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0; spacing: 5
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "RMUL 2027 · 3V3"; color: "#e9f7ff"; font.pixelSize: 22; font.bold: true; font.letterSpacing: 2 }
        Text { objectName: "tacticalMatchClock"; anchors.horizontalCenter: parent.horizontalCenter; text: root.clockText; color: root.hasMatch && root.gameDataContext.is_paused ? "#ffd081" : "#ffffff"; font.family: "Consolas"; font.pixelSize: 42; font.bold: true }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.hasMatch ? (root.gameDataContext.is_paused ? "比赛暂停" : root.gameDataContext.gamePhase) + " · 第 " + root.gameDataContext.currentRound + " 局" : "等待比赛状态"
            color: "#b5c9da"; font.pixelSize: 14
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.hasMatch ? "累计比分  红 " + root.gameDataContext.redScore + " : " + root.gameDataContext.blueScore + " 蓝" : "5 分钟制"
            color: "#8fa8ba"; font.pixelSize: 13
        }
    }
}
