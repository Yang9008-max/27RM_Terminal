import QtQuick 2.15

Item {
    id: tacticalLargeMapOverlay

    objectName: "tacticalLargeMapOverlay"
    property var pageRoot: null
    readonly property real mapAspectRatio: 12 / 8
    readonly property real mapWidth: Math.min(width, height * mapAspectRatio)
    readonly property real mapHeight: mapWidth / mapAspectRatio
    readonly property real mapVisualScale: mapWidth / 1120

    Rectangle {
        anchors.fill: parent
        color: "#02070d"
    }

    MouseArea {
        objectName: "tacticalLargeMapInputBlocker"
        anchors.fill: parent
        z: 0
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        preventStealing: true
        onPressed: function(mouse) { mouse.accepted = true }
        onWheel: function(wheel) { wheel.accepted = true }
    }

    RmulFieldOverview {
        id: tacticalLargeMap

        objectName: "tacticalLargeMap"
        width: tacticalLargeMapOverlay.mapWidth
        height: tacticalLargeMapOverlay.mapHeight
        anchors.centerIn: parent
        z: 1
        visualScale: tacticalLargeMapOverlay.mapVisualScale
        threadedCanvasRendering: true
        viewMirrored: pageRoot ? pageRoot.tacticalMapViewMirrored : false
        pageRoot: tacticalLargeMapOverlay.pageRoot
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 8
        z: 2
        width: largeMapHintText.implicitWidth + 32
        height: largeMapHintText.implicitHeight + 16
        radius: 6
        color: Qt.rgba(0.02, 0.08, 0.12, 0.86)
        border.color: "#4fd8ff"
        border.width: 1

        Text {
            id: largeMapHintText

            anchors.centerIn: parent
            text: "M 返回普通指挥屏"
            color: "#effbff"
            font.pixelSize: 14
            font.bold: true
        }
    }
}
