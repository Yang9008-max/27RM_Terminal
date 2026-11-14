// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "components"

Rectangle {
    id: root
    anchors.fill: parent
    color: "#02070d"
    signal loginButtonClicked()
    readonly property var gameDataContext: typeof gameData !== "undefined" ? gameData : null
    readonly property var mainWindowContext: typeof mainWindow !== "undefined" ? mainWindow : null
    readonly property var tacticalAnalyzerContext: typeof tacticalAnalyzer !== "undefined" ? tacticalAnalyzer : null
    // No fallback to TacticalMock: an absent packet must stay absent.
    readonly property var emptyTeam: ({controlPoints: null, damage: null, health: null, robots: []})
    readonly property var rmulData: gameDataContext && gameDataContext.rmulTacticalData !== undefined ? gameDataContext.rmulTacticalData : ({red: emptyTeam, blue: emptyTeam})
    readonly property bool allyIsBlue: gameDataContext ? Number(gameDataContext.currentRobotId) >= 100 : false
    readonly property bool allyIsRed: !allyIsBlue
    readonly property bool enemyIsBlue: !allyIsBlue
    readonly property var ally: allyIsBlue ? rmulData.blue : rmulData.red
    readonly property var enemy: allyIsBlue ? rmulData.red : rmulData.blue
    readonly property var allyRobotsForMap: ally.robots
    readonly property var enemyRobotsForMap: enemy.robots
    readonly property bool tacticalMapViewMirrored: allyIsBlue
    readonly property string tacticalMapBackgroundSource: ""
    readonly property var tacticalMapModel: ({allyRobots: [], enemyRobots: []})
    readonly property bool largeMapMode: mainWindowContext ? Boolean(mainWindowContext.tacticalLargeMapMode) : false
    readonly property bool largeMapOverlayReady: tacticalLargeMapLoader.active && tacticalLargeMapLoader.status === Loader.Ready && tacticalLargeMapLoader.item !== null && tacticalLargeMapLoader.visible
    readonly property string layoutMode: tacticalAnalyzerContext ? tacticalAnalyzerContext.layoutMode : "map_primary"
    readonly property bool videoPrimary: layoutMode === "video_primary"
    property string selectedRobotId: ""
    property string selectedEnemyId: ""
    function selectRobot(id, isEnemy) { selectedRobotId = isEnemy ? "" : id; selectedEnemyId = isEnemy ? id : "" }
    function clearSelection() { selectedRobotId = ""; selectedEnemyId = "" }

    property bool cameraRefreshEnabled: false
    property bool heroCameraGridVisible: false
    property real heroCameraFps: 0
    property real heroCameraLastRevision: 0
    property bool heroCameraFpsInitialized: false
    readonly property bool liveCameraSourceAllowed: cameraRefreshEnabled && !largeMapMode
    readonly property bool hasHeroCameraFrame: liveCameraSourceAllowed && gameDataContext !== null && Boolean(gameDataContext.hasHeroFrame)
    readonly property string heroCameraFrameSource: hasHeroCameraFrame ? String(gameDataContext.heroFrameSource) : ""
    function resetHeroCameraFpsMeter() { heroCameraFps = 0; heroCameraLastRevision = 0; heroCameraFpsInitialized = false }
    function sampleHeroCameraFps() {
        if (!hasHeroCameraFrame) { resetHeroCameraFpsMeter(); return }
        var revision = Number(gameDataContext.heroFrameRevision)
        heroCameraFps = heroCameraFpsInitialized ? Math.max(0, revision - heroCameraLastRevision) : 0
        heroCameraFpsInitialized = true
        heroCameraLastRevision = revision
    }
    onLiveCameraSourceAllowedChanged: resetHeroCameraFpsMeter()
    Timer { interval: 1000; repeat: true; running: root.liveCameraSourceAllowed; onTriggered: root.sampleHeroCameraFps() }

    readonly property real designScale: Math.max(0.01, Math.min(width / 1920, height / 1080))
    Item {
        id: surface
        width: 1920; height: 1080
        x: (root.width - width * root.designScale) / 2
        y: (root.height - height * root.designScale) / 2
        scale: root.designScale; transformOrigin: Item.TopLeft

        RmulTacticalHeader {
            objectName: "rmulTacticalHeader"
            x: 24; y: 18; width: 1872; height: 144
            gameDataContext: root.gameDataContext
            ally: root.ally; enemy: root.enemy; allyIsBlue: root.allyIsBlue
        }
        Text {
            x: 28; y: 176
            text: "重装 1 · 步兵 3 · 哨兵 7     /     " + (root.gameDataContext && root.gameDataContext.hasMatchStatus ? "比赛状态已接收" : "等待比赛状态")
            color: "#82a1b9"; font.pixelSize: 13
        }
        Button {
            objectName: "tacticalSettingsButton"
            x: 1736; y: 169; width: 160; height: 32
            text: "登录 / 设置  P"
            onClicked: { if (root.mainWindowContext) root.mainWindowContext.toggleSettingsPanel(); root.loginButtonClicked() }
        }
        RmulRosterPanel { objectName: "tacticalAllyRoster"; x: 24; y: 216; width: 360; height: 504; robots: root.ally.robots; blue: root.allyIsBlue }
        RmulRosterPanel { objectName: "tacticalEnemyRoster"; x: 1536; y: 216; width: 360; height: 504; robots: root.enemy.robots; blue: root.enemyIsBlue; enemy: true }

        RmulFieldOverview {
            objectName: "tacticalCenterMap"
            x: 400; y: 216; width: 1120; height: 504
            visible: !root.videoPrimary && !root.largeMapMode
            viewMirrored: root.allyIsBlue; pageRoot: root
        }
        CameraPreviewPanel {
            objectName: "tacticalCenterCamera"
            x: 400; y: 216; width: 1120; height: 504
            visible: root.videoPrimary && !root.largeMapMode
            centralMode: true; showRealVideo: visible
            useDefaultImage: false
            showGrid: root.heroCameraGridVisible
            liveFrameSource: root.heroCameraFrameSource
            liveFps: root.hasHeroCameraFrame ? root.heroCameraFps : -1
        }
        HudPanel {
            objectName: "rmulTacticalNotes"
            x: 24; y: 736; width: 360; height: 320
            title: "操作与数据"; accent: "#d9af62"; eyebrow: "RMUL"
            Column {
                x: 20; y: 56; width: parent.width - 40; spacing: 19
                Repeater {
                    model: ["M   场地示意 / 返回", "Ctrl+Y   地图 / 相机主画面", "Ctrl+T   切换操作页", "P   登录与设置", "血量归零与未收到数据分别显示", "位置与占领状态尚未接入"]
                    Text { required property string modelData; width: parent.width; text: modelData; color: "#a8c2d6"; font.pixelSize: 14; wrapMode: Text.WordWrap }
                }
            }
        }
        RmulComparisonPanel { objectName: "rmulTacticalComparison"; x: 400; y: 736; width: 864; height: 320; ally: root.ally; enemy: root.enemy; allyIsBlue: root.allyIsBlue }
        CameraPreviewPanel {
            objectName: "tacticalAuxCamera"
            x: 1280; y: 736; width: 616; height: 320
            visible: !root.videoPrimary && !root.largeMapMode
            showRealVideo: visible; useDefaultImage: false
            showGrid: root.heroCameraGridVisible
            liveFrameSource: root.heroCameraFrameSource
            liveFps: root.hasHeroCameraFrame ? root.heroCameraFps : -1
        }
        RmulFieldOverview {
            objectName: "tacticalAuxMap"
            x: 1280; y: 736; width: 616; height: 320
            visible: root.videoPrimary && !root.largeMapMode
            viewMirrored: root.allyIsBlue; pageRoot: root
        }
    }
    Loader {
        id: tacticalLargeMapLoader
        objectName: "tacticalLargeMapLoader"
        anchors.fill: parent; z: 1000
        active: root.largeMapMode; visible: active
        source: "TacticalLargeMapOverlay.qml"
        onLoaded: item.pageRoot = root
    }
}
