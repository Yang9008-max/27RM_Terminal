// SPDX-License-Identifier: MIT
import QtQuick

Rectangle {
    id: root
    required property var gameDataContext
    required property int robotId
    required property string roleName
    property color teamColor: "#ef6473"
    objectName: "robotCard" + robotId
    implicitWidth: 144
    implicitHeight: 62
    radius: 6
    color: "#172332"
    border.color: "#2d3b4c"

    property var robotInfo: gameDataContext ? gameDataContext.getRobotInfo(robotId) : null
    readonly property bool hasHealth: robotInfo !== null && robotInfo.hasGlobalHealth === true
    readonly property string healthText: hasHealth ? "HP " + robotInfo.globalHealth : "HP —"

    Connections {
        target: root.gameDataContext
        function onRobotDataUpdated(id) {
            if (id === root.robotId)
                root.robotInfo = root.gameDataContext.getRobotInfo(root.robotId)
        }
    }

    Rectangle {
        x: 10
        y: 12
        width: 30
        height: 38
        radius: 4
        color: "#202f40"
        Text {
            anchors.centerIn: parent
            text: root.robotId % 100
            color: root.teamColor
            font.pixelSize: 23
            font.bold: true
        }
    }
    Text {
        x: 50
        y: 10
        width: parent.width - 56
        text: root.roleName
        color: "#dce5ef"
        font.pixelSize: 13
        elide: Text.ElideRight
    }
    Text {
        objectName: "robotHealth" + root.robotId
        x: 50
        y: 32
        width: parent.width - 56
        text: root.healthText
        color: !root.hasHealth ? "#8c9aac" : (root.robotInfo.globalHealth === 0 ? "#ffba70" : "#ffffff")
        font.pixelSize: 15
        font.bold: true
        elide: Text.ElideRight
    }
}
