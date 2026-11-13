// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    required property var gameDataContext
    property bool blue: false
    readonly property color teamColor: blue ? "#62baff" : "#ff7180"
    readonly property bool hasControlPoints: gameDataContext !== null &&
        (blue ? gameDataContext.hasBlueControlPoints : gameDataContext.hasRedControlPoints)
    // Keep uint32 values as JS numbers; a QML int would overflow above INT_MAX.
    readonly property var controlPoints: gameDataContext ?
        (blue ? gameDataContext.blueControlPoints : gameDataContext.redControlPoints) : 0
    color: "#e6101c29"
    radius: 8
    border.color: blue ? "#31516b" : "#653e4a"

    Rectangle {
        x: 0
        y: 16
        width: 3
        height: 36
        radius: 1
        color: root.teamColor
    }
    Text {
        x: 20
        y: 18
        text: root.blue ? "蓝方" : "红方"
        color: root.teamColor
        font.pixelSize: 19
        font.bold: true
    }
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 20
        y: 10
        spacing: 14
        Text {
            anchors.baseline: cp.baseline
            text: "控制度"
            color: "#aab8c9"
            font.pixelSize: 13
        }
        Text {
            id: cp
            objectName: root.blue ? "blueControlPointsText" : "redControlPointsText"
            text: root.hasControlPoints ? String(root.controlPoints) : "—"
            color: root.hasControlPoints ? root.teamColor : "#8c9aac"
            font.pixelSize: 34
            font.bold: true
        }
    }
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 14
        spacing: 10
        RmulRobotCard {
            Layout.fillWidth: true
            gameDataContext: root.gameDataContext
            robotId: root.blue ? 101 : 1
            roleName: "重装"
            teamColor: root.teamColor
        }
        RmulRobotCard {
            Layout.fillWidth: true
            gameDataContext: root.gameDataContext
            robotId: root.blue ? 103 : 3
            roleName: "步兵"
            teamColor: root.teamColor
        }
        RmulRobotCard {
            Layout.fillWidth: true
            gameDataContext: root.gameDataContext
            robotId: root.blue ? 107 : 7
            roleName: "哨兵"
            teamColor: root.teamColor
        }
    }
}
