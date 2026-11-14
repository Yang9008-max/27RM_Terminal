// SPDX-License-Identifier: MIT
// Original schematic based on the 2027 RMUL rulebook, figures 3-1 and 3-4.
// Relative structure only: no survey coordinates, occupancy or robot locations.
import QtQuick
import "components"

HudPanel {
    id: root
    property bool viewMirrored: false
    property bool threadedCanvasRendering: false
    property real visualScale: 1
    property var pageRoot: null
    readonly property real fieldAspectRatio: 12 / 8
    readonly property bool hasLivePositions: false
    title: "2027 RMUL · 场地结构"
    eyebrow: "12 m × 8 m"
    accent: "#42b9d8"
    showDiagonalTexture: false

    Item {
        id: field
        width: Math.min(root.width - 56, (root.height - 112) * root.fieldAspectRatio)
        height: width / root.fieldAspectRatio
        anchors.horizontalCenter: parent.horizontalCenter
        y: 46
        rotation: root.viewMirrored ? 180 : 0
        Rectangle { anchors.fill: parent; color: "#07131e"; border.color: "#436070"; border.width: 3 }
        Canvas {
            id: fieldGrid
            anchors.fill: parent
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#142b3b"; ctx.lineWidth = 1
                for (var i = 1; i < 12; ++i) { ctx.beginPath(); ctx.moveTo(width * i / 12, 0); ctx.lineTo(width * i / 12, height); ctx.stroke() }
                for (var j = 1; j < 8; ++j) { ctx.beginPath(); ctx.moveTo(0, height * j / 8); ctx.lineTo(width, height * j / 8); ctx.stroke() }
            }
        }
        Repeater {
            model: [
                { x: 0.025, y: 0.035, w: 0.2, h: 0.22, label: "红方补给区", color: "#ff5366" },
                { x: 0.225, y: 0.15, w: 0.16, h: 0.47, label: "红方高地\n控制区", color: "#4ac6b1" },
                { x: 0.385, y: 0.32, w: 0.24, h: 0.34, label: "中央控制区", color: "#5cddb5" },
                { x: 0.625, y: 0.38, w: 0.16, h: 0.47, label: "蓝方高地\n控制区", color: "#4ac6b1" },
                { x: 0.785, y: 0.745, w: 0.19, h: 0.22, label: "蓝方补给区", color: "#39b8ff" }
            ]
            Rectangle {
                id: zone
                required property var modelData
                readonly property color zoneColor: modelData.color
                x: modelData.x * field.width; y: modelData.y * field.height
                width: modelData.w * field.width; height: modelData.h * field.height
                color: Qt.rgba(zoneColor.r, zoneColor.g, zoneColor.b, 0.10)
                border.color: modelData.color; border.width: 1
                Text {
                    anchors.centerIn: parent; rotation: root.viewMirrored ? 180 : 0
                    text: zone.modelData.label; color: zone.modelData.color
                    font.pixelSize: Math.max(9, Math.min(16, field.width / 42)); font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
    Text {
        objectName: "rmulFieldNotice"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 17
        text: "结构示意 · 非测绘地图 · 未接入实时定位 / 占领状态"
        color: "#8ba3b6"; font.pixelSize: Math.max(10, Math.min(14, root.width / 60))
    }
}
