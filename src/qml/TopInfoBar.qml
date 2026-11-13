// SPDX-License-Identifier: MIT
// RMUL 2027 3V3 public HUD. Received data only; no local match simulation.
import QtQuick
import QtQuick.Layouts

Item {
    id: root
    width: parent ? parent.width : 1920
    height: 170
    readonly property var gameDataContext: typeof gameData !== "undefined" ? gameData : null
    readonly property bool hasMatchStatus: gameDataContext !== null && gameDataContext.hasMatchStatus
    readonly property string formattedTime: {
        if (!root.hasMatchStatus)
            return "--:--";
        var seconds = Math.max(0, Math.floor(Number(root.gameDataContext.remainingTime)));
        return String(Math.floor(seconds / 60)).padStart(2, "0") + ":" + String(seconds % 60).padStart(2, "0");
    }
    property bool showPerformanceOverlay: false
    // MainWindow allocates as little as 136 logical pixels on compact/high-DPI
    // desktops. Keep the complete 170px design readable instead of overlapping
    // the control-point heading with the cards when the viewport shrinks.
    readonly property real contentScale: Math.max(0.01, Math.min(1, root.height / 170))

    Item {
        width: root.width / root.contentScale
        height: 170
        scale: root.contentScale
        transformOrigin: Item.TopLeft

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 16

            RmulTeamPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                gameDataContext: root.gameDataContext
            }

            Item {
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 4
                    text: "RMUL 2027"
                    color: "#edf5ff"
                    font.pixelSize: 18
                    font.bold: true
                    font.letterSpacing: 2
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 32
                    text: "3V3 · 5 分钟制"
                    color: "#9eb0c6"
                    font.pixelSize: 12
                }
                Text {
                    objectName: "matchClockText"
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 53
                    text: root.formattedTime
                    color: root.hasMatchStatus && root.gameDataContext.is_paused ? "#ffcc70" : "#ffffff"
                    font.family: "Consolas"
                    font.pixelSize: 38
                    font.bold: true
                }
                Text {
                    objectName: "matchPhaseText"
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 108
                    text: root.hasMatchStatus ? (root.gameDataContext.is_paused ? "比赛暂停" : root.gameDataContext.gamePhase) + " · 第 " + root.gameDataContext.currentRound + " 局" : "等待比赛数据"
                    color: "#b4c5d9"
                    font.pixelSize: 12
                }
            }

            RmulTeamPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                gameDataContext: root.gameDataContext
                blue: true
            }
        }
    }

    FpsDisplay {
        visible: root.showPerformanceOverlay
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }
}
