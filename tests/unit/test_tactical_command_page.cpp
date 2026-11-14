#include <QJSValue>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickImageProvider>
#include <QtTest>
#include "core/GameData.h"
#include "core/TacticalAnalyzer.h"
#include "robomaster.pb.h"

namespace {
QQuickItem *visualItem(QQuickItem *root, const QString &name) {
  if (root->objectName() == name) return root;
  for (auto *child : root->childItems())
    if (auto *found = visualItem(child, name)) return found;
  return nullptr;
}
class FrameProvider : public QQuickImageProvider {
public:
  FrameProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}
  QImage requestImage(const QString &, QSize *size, const QSize &) override {
    QImage frame(16, 16, QImage::Format_RGB32);
    frame.fill(Qt::green);
    if (size) *size = frame.size();
    return frame;
  }
};
}

class TacticalMainWindowStub : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool tacticalLargeMapMode READ tacticalLargeMapMode WRITE
                 setTacticalLargeMapMode NOTIFY tacticalLargeMapModeChanged)

public:
  bool tacticalLargeMapMode() const { return m_tacticalLargeMapMode; }

  void setTacticalLargeMapMode(bool enabled) {
    if (m_tacticalLargeMapMode == enabled) {
      return;
    }
    m_tacticalLargeMapMode = enabled;
    emit tacticalLargeMapModeChanged();
  }

signals:
  void tacticalLargeMapModeChanged();

private:
  bool m_tacticalLargeMapMode = false;
};

class TacticalGameDataStub : public QObject {
  Q_OBJECT
  Q_PROPERTY(int currentRobotId READ currentRobotId WRITE setCurrentRobotId NOTIFY
                 currentRobotIdChanged)
  Q_PROPERTY(bool hasHeroFrame READ hasHeroFrame NOTIFY heroFrameUpdated)
  Q_PROPERTY(quint64 heroFrameRevision READ heroFrameRevision NOTIFY
                 heroFrameUpdated)
  Q_PROPERTY(QString heroFrameSource READ heroFrameSource NOTIFY
                 heroFrameUpdated)

public:
  int currentRobotId() const { return m_currentRobotId; }
  bool hasHeroFrame() const { return m_hasHeroFrame; }
  quint64 heroFrameRevision() const { return m_heroFrameRevision; }
  QString heroFrameSource() const {
    return m_hasHeroFrame
               ? QStringLiteral("image://herovideo/frame?rev=%1")
                     .arg(m_heroFrameRevision)
               : QString();
  }

  void setCurrentRobotId(int robotId) {
    if (m_currentRobotId == robotId) {
      return;
    }
    m_currentRobotId = robotId;
    emit currentRobotIdChanged();
  }

  void publishHeroFrame() {
    m_hasHeroFrame = true;
    ++m_heroFrameRevision;
    emit heroFrameUpdated();
  }

signals:
  void currentRobotIdChanged();
  void heroFrameUpdated();

private:
  int m_currentRobotId = 1;
  bool m_hasHeroFrame = false;
  quint64 m_heroFrameRevision = 0;
};

class TestTacticalCommandPage : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    Q_INIT_RESOURCE(qml);
    Q_INIT_RESOURCE(resources);
  }

  void cleanupTestCase() {
    Q_CLEANUP_RESOURCE(resources);
    Q_CLEANUP_RESOURCE(qml);
  }

  void rmulReceivedStateAndFixedRoster() {
    GameData data;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("gameData", &data);
    QQmlComponent component(&engine, QUrl("qrc:/qml/Tactical/TacticalCommandPage.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> page(component.create());
    QVERIFY(page);
    QVERIFY2(page->property("rmulData").isValid(), "Tactical screen still uses the legacy projection");
    auto summary = [&data]() { return data.property("rmulTacticalData").toMap(); };
    QVERIFY(!summary().value("red").toMap().value("controlPoints").isValid());
    QVERIFY(!summary().value("red").toMap().value("health").isValid());

    robomaster::GlobalUnitStatus msg;
    for (int hp : {500, 123, 999, 300, 450, 350, 888, 250}) msg.add_robot_health(hp);
    msg.set_total_cp_ally(600);
    msg.set_total_cp_enemy(590);
    msg.set_total_damage_ally(0);
    msg.set_total_damage_enemy(1234);
    data.updateGlobalUnitStatus(msg);
    auto red = summary().value("red").toMap();
    auto blue = summary().value("blue").toMap();
    QCOMPARE(red.value("health").toUInt(), 923u);
    QCOMPARE(blue.value("health").toUInt(), 1050u);
    QCOMPARE(red.value("damage").toUInt(), 0u);
    QCOMPARE(blue.value("damage").toUInt(), 1234u);
    const auto roster = red.value("robots").toList();
    QCOMPARE(roster.size(), 3);
    for (int i = 0; i < 3; ++i) {
      QCOMPARE(roster[i].toMap().value("slot").toInt(), QList<int>({1,3,7})[i]);
      QVERIFY(roster[i].toMap().value("label").toString() != "英雄");
    }
    auto *header = page->findChild<QObject *>("rmulTacticalHeader");
    QVERIFY(header);
    QTRY_COMPARE(header->property("allyControlText").toString(), QString("600"));
    msg.set_total_cp_ally(0);
    msg.set_robot_health(1, 0);
    data.updateGlobalUnitStatus(msg);
    QTRY_COMPARE(header->property("allyControlText").toString(), QString("0"));
    QCOMPARE(summary().value("red").toMap().value("health").toUInt(), 800u);
    data.updateGlobalUnitStatus(robomaster::GlobalUnitStatus{});
    QCOMPARE(header->property("allyControlText").toString(), QString("0"));
    data.resetGlobalUnitStatus();
    QTRY_COMPARE(header->property("allyControlText").toString(), QString("—"));
    QVERIFY(!summary().value("red").toMap().value("health").isValid());
  }

  void rmulBluePerspectiveAndPartialHealth() {
    GameData data;
    data.setCurrentRobotId(103);
    robomaster::GlobalUnitStatus msg;
    msg.set_total_cp_ally(0xffffffffu);
    msg.set_total_cp_enemy(0);
    msg.set_total_damage_ally(0xffffffffu);
    msg.add_robot_health(0);
    data.updateGlobalUnitStatus(msg);
    auto summary = data.property("rmulTacticalData").toMap();
    QVERIFY2(!summary.isEmpty(), "No RMUL tactical projection");
    auto blue = summary.value("blue").toMap();
    QCOMPARE(blue.value("controlPoints").toULongLong(), 0xffffffffull);
    QCOMPARE(blue.value("damage").toULongLong(), 0xffffffffull);
    // An invalid partial HP array must never invent a complete team total.
    QVERIFY(!blue.value("health").isValid());
    data.setCurrentRobotId(1);
    summary = data.property("rmulTacticalData").toMap();
    QVERIFY(!summary.value("blue").toMap().value("controlPoints").isValid());
  }

  void rmulVisibleRosterClockAndLayoutSwitch() {
    GameData data;
    data.setCurrentRobotId(103);
    RM::TacticalAnalyzer analyzer(&data);
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("gameData", &data);
    engine.rootContext()->setContextProperty("tacticalAnalyzer", &analyzer);
    QQmlComponent component(&engine, QUrl("qrc:/qml/Tactical/TacticalCommandPage.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *page = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(page);
    page->setWidth(1280); page->setHeight(800);
    robomaster::GlobalUnitStatus msg;
    for (int hp : {0, 123, 999, 300, 450, 350, 888, 250}) msg.add_robot_health(hp);
    msg.set_total_cp_ally(0xffffffffu); msg.set_total_cp_enemy(0);
    data.updateGlobalUnitStatus(msg);
    for (int id : {1, 3, 7, 101, 103, 107}) {
      auto *card = visualItem(page, QString("tacticalRobot%1").arg(id));
      QVERIFY(card);
      auto bounds = card->mapRectToScene(card->boundingRect());
      QVERIFY(bounds.left() >= 0 && bounds.right() <= 1280);
      QVERIFY(bounds.top() >= 0 && bounds.bottom() <= 800);
    }
    for (int id : {2, 4, 6, 102, 104, 106})
      QVERIFY(!visualItem(page, QString("tacticalRobot%1").arg(id)));
    QTRY_COMPARE(visualItem(page, "tacticalHealth101")->property("text").toString(), QString("0"));
    QCOMPARE(visualItem(page, "tacticalHealth3")->property("text").toString(), QString("350"));
    QCOMPARE(visualItem(page, "tacticalAllyControlPoints")->property("text").toString(), QString("4294967295"));
    auto *clock = visualItem(page, "tacticalMatchClock");
    QVERIFY(clock);
    QCOMPARE(clock->property("text").toString(), QString("--:--"));
    robomaster::GameStatus status;
    status.set_current_stage(4); status.set_current_round(1); status.set_stage_countdown_sec(300);
    data.updateGameStatus(status);
    QTRY_COMPARE(clock->property("text").toString(), QString("05:00"));
    status.set_is_paused(true); status.set_stage_countdown_sec(0);
    data.updateGameStatus(status);
    QCOMPARE(clock->property("text").toString(), QString("00:00"));
    auto *centerMap = visualItem(page, "tacticalCenterMap");
    auto *auxMap = visualItem(page, "tacticalAuxMap");
    auto *camera = visualItem(page, "tacticalCenterCamera");
    QVERIFY(centerMap && auxMap && camera);
    QVERIFY(centerMap->isVisible() && !auxMap->isVisible() && !camera->isVisible());
    analyzer.setLayoutMode("video_primary");
    QTRY_VERIFY(!centerMap->isVisible() && auxMap->isVisible() && camera->isVisible());
    data.resetGlobalUnitStatus();
    QTRY_COMPARE(visualItem(page, "tacticalHealth101")->property("text").toString(), QString("—"));
    QCOMPARE(visualItem(page, "tacticalAllyControlPoints")->property("text").toString(), QString("—"));
  }

  void testLargeMapOverlayLifecycleAndGeometry_data() {
    QTest::addColumn<int>("pageWidth");
    QTest::addColumn<int>("pageHeight");

    // 下列数据仅验证跨平台一致的 Qt 逻辑坐标；Ubuntu 上的
    // X11/Wayland 场景图仍需在目标机器单独验证。
    QTest::newRow("low-resolution-4-by-3") << 800 << 600;
    QTest::newRow("hd-16-by-9") << 1280 << 720;
    QTest::newRow("laptop-near-16-by-9") << 1366 << 768;
    QTest::newRow("full-hd") << 1920 << 1080;
    QTest::newRow("16-by-10") << 1920 << 1200;
    QTest::newRow("qhd") << 2560 << 1440;
    QTest::newRow("ultrawide") << 3440 << 1440;
    QTest::newRow("4k") << 3840 << 2160;
  }

  void testLargeMapOverlayLifecycleAndGeometry() {
    QFETCH(int, pageWidth);
    QFETCH(int, pageHeight);

    QQmlEngine engine;
    TacticalMainWindowStub mainWindow;
    engine.rootContext()->setContextProperty(QStringLiteral("mainWindow"),
                                             &mainWindow);

    QQmlComponent component(
        &engine,
        QUrl(QStringLiteral("qrc:/qml/Tactical/TacticalCommandPage.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> pageObject(component.create());
    QVERIFY2(pageObject, qPrintable(component.errorString()));
    auto *page = qobject_cast<QQuickItem *>(pageObject.data());
    QVERIFY(page);
    page->setWidth(pageWidth);
    page->setHeight(pageHeight);

    QCOMPARE(page->property("largeMapMode").toBool(), false);
    QCOMPARE(page->property("largeMapOverlayReady").toBool(), false);
    const QString originalLayoutMode = page->property("layoutMode").toString();
    page->setProperty("selectedRobotId", QStringLiteral("A3"));

    QObject *loader =
        page->findChild<QObject *>(QStringLiteral("tacticalLargeMapLoader"));
    QVERIFY(loader);
    auto *centerMap = qobject_cast<QQuickItem *>(
        page->findChild<QObject *>(QStringLiteral("tacticalCenterMap")));
    auto *auxMap = qobject_cast<QQuickItem *>(
        page->findChild<QObject *>(QStringLiteral("tacticalAuxMap")));
    QVERIFY(centerMap);
    QVERIFY(auxMap);
    QCOMPARE(centerMap->property("visualScale").toReal(), 1.0);
    QCOMPARE(auxMap->property("visualScale").toReal(), 1.0);
    QVERIFY(!centerMap->property("threadedCanvasRendering").toBool());
    QVERIFY(!auxMap->property("threadedCanvasRendering").toBool());
    QVERIFY(centerMap->isVisible());
    QVERIFY(!auxMap->isVisible());
    QCOMPARE(loader->property("active").toBool(), false);
    QVERIFY(
        !page->findChild<QObject *>(QStringLiteral("tacticalLargeMapOverlay")));

    mainWindow.setTacticalLargeMapMode(true);
    QTRY_VERIFY(page->property("largeMapMode").toBool());
    QTRY_VERIFY(page->property("largeMapOverlayReady").toBool());

    QObject *overlayObject = nullptr;
    QTRY_VERIFY((overlayObject = page->findChild<QObject *>(
                     QStringLiteral("tacticalLargeMapOverlay"))) != nullptr);
    auto *overlay = qobject_cast<QQuickItem *>(overlayObject);
    QVERIFY(overlay);
    QTRY_COMPARE(overlay->width(), static_cast<qreal>(pageWidth));
    QTRY_COMPARE(overlay->height(), static_cast<qreal>(pageHeight));
    QVERIFY(!centerMap->isVisible());
    QVERIFY(!auxMap->isVisible());

    auto *inputBlocker = qobject_cast<QQuickItem *>(page->findChild<QObject *>(
        QStringLiteral("tacticalLargeMapInputBlocker")));
    QVERIFY(inputBlocker);
    QCOMPARE(inputBlocker->width(), overlay->width());
    QCOMPARE(inputBlocker->height(), overlay->height());

    QObject *largeMapObject =
        page->findChild<QObject *>(QStringLiteral("tacticalLargeMap"));
    auto *largeMap = qobject_cast<QQuickItem *>(largeMapObject);
    QVERIFY(largeMap);
    QCOMPARE(largeMap->property("pageRoot").value<QObject *>(),
             pageObject.data());
    QCOMPARE(largeMap->scale(), 1.0);
    QCOMPARE(largeMap->z(), 1.0);
    QVERIFY(largeMap->property("threadedCanvasRendering").toBool());
    QVERIFY(largeMap->z() > inputBlocker->z());

    const QRectF mappedBounds = largeMap->mapRectToItem(
        page, QRectF(0.0, 0.0, largeMap->width(), largeMap->height()));
    QVERIFY(mappedBounds.left() >= -1.0);
    QVERIFY(mappedBounds.top() >= -1.0);
    QVERIFY(mappedBounds.right() <= page->width() + 1.0);
    QVERIFY(mappedBounds.bottom() <= page->height() + 1.0);
    const bool fillsWidth = qAbs(mappedBounds.width() - page->width()) <= 10.0;
    const bool fillsHeight =
        qAbs(mappedBounds.height() - page->height()) <= 1.0;
    QVERIFY2(fillsWidth || fillsHeight,
             "The aspect-preserving map must fill at least one screen axis");
    QCOMPARE(largeMap->width(), mappedBounds.width());
    QCOMPARE(largeMap->height(), mappedBounds.height());
    const qreal expectedWidth =
        qMin(static_cast<qreal>(pageWidth),
             static_cast<qreal>(pageHeight) * 12.0 / 8.0);
    const qreal expectedHeight = expectedWidth * 8.0 / 12.0;
    QVERIFY(qAbs(largeMap->width() - expectedWidth) <= 0.001);
    QVERIFY(qAbs(largeMap->height() - expectedHeight) <= 0.001);
    QVERIFY(qAbs(largeMap->property("visualScale").toReal() -
                 largeMap->width() / 1120.0) <= 0.001);

    QCOMPARE(largeMap->property("hasLivePositions").toBool(), false);
    QCOMPARE(largeMap->property("fieldAspectRatio").toReal(), 1.5);

    QPointer<QObject> destroyedWithLoader = overlayObject;
    mainWindow.setTacticalLargeMapMode(false);
    QTRY_VERIFY(!page->property("largeMapMode").toBool());
    QTRY_VERIFY(!page->property("largeMapOverlayReady").toBool());
    QTRY_VERIFY(destroyedWithLoader.isNull());
    QVERIFY(centerMap->isVisible());
    QVERIFY(!auxMap->isVisible());
    QCOMPARE(page->property("layoutMode").toString(), originalLayoutMode);
    QCOMPARE(page->property("selectedRobotId").toString(),
             QStringLiteral("A3"));
  }

  void testLargeMapUsesSameCampViewMappingAsTacticalMaps() {
    QQmlEngine engine;
    TacticalMainWindowStub mainWindow;
    TacticalGameDataStub gameData;
    gameData.setCurrentRobotId(101);
    engine.rootContext()->setContextProperty(QStringLiteral("mainWindow"),
                                             &mainWindow);
    engine.rootContext()->setContextProperty(QStringLiteral("gameData"),
                                             &gameData);

    QQmlComponent component(
        &engine,
        QUrl(QStringLiteral("qrc:/qml/Tactical/TacticalCommandPage.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> pageObject(component.create());
    QVERIFY2(pageObject, qPrintable(component.errorString()));
    auto *page = qobject_cast<QQuickItem *>(pageObject.data());
    QVERIFY(page);
    page->setWidth(1920);
    page->setHeight(1080);

    QCOMPARE(page->property("allyIsBlue").toBool(), true);
    QCOMPARE(page->property("tacticalMapViewMirrored").toBool(), true);
    QCOMPARE(page->property("tacticalMapBackgroundSource").toString(), QString());
    auto *centerMap = page->findChild<QObject *>("tacticalCenterMap");
    QVERIFY(centerMap);
    QCOMPARE(centerMap->property("viewMirrored").toBool(), true);
    QCOMPARE(centerMap->property("hasLivePositions").toBool(), false);
    mainWindow.setTacticalLargeMapMode(true);
    QTRY_VERIFY(page->property("largeMapOverlayReady").toBool());
    auto *largeMap = page->findChild<QObject *>("tacticalLargeMap");
    QVERIFY(largeMap);
    QCOMPARE(largeMap->property("viewMirrored").toBool(), true);
    QCOMPARE(largeMap->property("fieldAspectRatio").toReal(), 1.5);
    gameData.setCurrentRobotId(1);
    QTRY_COMPARE(largeMap->property("viewMirrored").toBool(), false);
  }

  void testCameraRefreshUsesHeroFrameRevisionDirectly() {
    QQmlEngine engine;
    engine.addImageProvider("herovideo", new FrameProvider);
    TacticalMainWindowStub mainWindow;
    TacticalGameDataStub gameData;
    engine.rootContext()->setContextProperty(QStringLiteral("mainWindow"),
                                             &mainWindow);
    engine.rootContext()->setContextProperty(QStringLiteral("gameData"),
                                             &gameData);

    QQmlComponent component(
        &engine,
        QUrl(QStringLiteral("qrc:/qml/Tactical/TacticalCommandPage.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> pageObject(component.create());
    QVERIFY2(pageObject, qPrintable(component.errorString()));
    QObject *page = pageObject.data();

    gameData.publishHeroFrame();
    QCOMPARE(page->property("heroCameraFrameSource").toString(), QString());

    QVERIFY(page->setProperty("cameraRefreshEnabled", true));
    QTRY_COMPARE(page->property("heroCameraFrameSource").toString(),
                 gameData.heroFrameSource());

    QVERIFY(QMetaObject::invokeMethod(page, "sampleHeroCameraFps"));
    for (int i = 0; i < 7; ++i) {
      gameData.publishHeroFrame();
    }
    QVERIFY(QMetaObject::invokeMethod(page, "sampleHeroCameraFps"));
    QCOMPARE(page->property("heroCameraFps").toReal(), 7.0);

    QObject *camera =
        page->findChild<QObject *>(QStringLiteral("tacticalAuxCamera"));
    QVERIFY(camera);
    QTRY_COMPARE(camera->property("liveFrameSource").toString(),
                 gameData.heroFrameSource());
    QCOMPARE(camera->property("liveFps").toReal(), 7.0);
    QCOMPARE(camera->property("showGrid").toBool(), false);

    mainWindow.setTacticalLargeMapMode(true);
    QTRY_COMPARE(page->property("heroCameraFrameSource").toString(), QString());
    QTRY_COMPARE(camera->property("liveFrameSource").toString(), QString());
    QTRY_VERIFY(!camera->property("visible").toBool());
    mainWindow.setTacticalLargeMapMode(false);
    QTRY_COMPARE(camera->property("liveFrameSource").toString(),
                 gameData.heroFrameSource());
    QTRY_VERIFY(camera->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(page, "sampleHeroCameraFps"));
    QCOMPARE(page->property("heroCameraFps").toReal(), 0.0);
    for (int i = 0; i < 3; ++i) {
      gameData.publishHeroFrame();
    }
    QVERIFY(QMetaObject::invokeMethod(page, "sampleHeroCameraFps"));
    QCOMPARE(page->property("heroCameraFps").toReal(), 3.0);

    QVERIFY(page->setProperty("heroCameraGridVisible", true));
    QTRY_VERIFY(camera->property("showGrid").toBool());
  }
};

QTEST_MAIN(TestTacticalCommandPage)
#include "test_tactical_command_page.moc"
