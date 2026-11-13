#include "core/GameData.h"
#include "network/NetworkManager.h"
#include "robomaster.pb.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QSaveFile>
#include <QtTest>

namespace {
QQuickItem *item(QQuickItem *root, const QString &name) {
  if (root->objectName() == name) return root;
  for (auto *child : root->childItems()) {
    if (auto *found = item(child, name)) return found;
  }
  return nullptr;
}
QString text(QQuickView &view, const QString &name) {
  auto *found = item(view.rootObject(), name);
  return found ? found->property("text").toString() : QStringLiteral("<missing>");
}
void load(QQuickView &view, GameData &data, int width = 1920, int height = 170) {
  view.rootContext()->setContextProperty("gameData", &data);
  view.setResizeMode(QQuickView::SizeRootObjectToView);
  view.setColor(QColor("#09121c"));
  view.setSource(QUrl("qrc:/qml/TopInfoBar.qml"));
  view.resize(width, height);
  view.show();
  QCoreApplication::processEvents();
}
robomaster::GlobalUnitStatus healthPacket(int hp = 400) {
  robomaster::GlobalUnitStatus msg;
  for (int i = 0; i < 8; ++i) msg.add_robot_health(hp + i);
  return msg;
}
}

class TestRmul2027Hud : public QObject {
  Q_OBJECT
private slots:
  void initTestCase() { Q_INIT_RESOURCE(qml); Q_INIT_RESOURCE(resources); }
  void unknownThenReceivedZeroAndMissing() {
    GameData data;
    QQuickView view;
    load(view, data);
    QCOMPARE(view.status(), QQuickView::Ready);
    QTRY_COMPARE(text(view, "redControlPointsText"), QStringLiteral("—"));
    QCOMPARE(text(view, "blueControlPointsText"), QStringLiteral("—"));
    QCOMPARE(text(view, "robotHealth1"), QStringLiteral("HP —"));
    QCOMPARE(text(view, "matchClockText"), QStringLiteral("--:--"));
    auto msg = healthPacket();
    msg.set_total_cp_ally(600);
    msg.set_total_cp_enemy(590);
    data.updateGlobalUnitStatus(msg);
    QTRY_COMPARE(text(view, "redControlPointsText"), QStringLiteral("600"));
    QCOMPARE(text(view, "blueControlPointsText"), QStringLiteral("590"));
    QCOMPARE(text(view, "robotHealth3"), QStringLiteral("HP 401"));
    msg.set_total_cp_ally(0);
    msg.set_robot_health(1, 0);
    data.updateGlobalUnitStatus(msg);
    QTRY_COMPARE(text(view, "redControlPointsText"), QStringLiteral("0"));
    QCOMPARE(text(view, "robotHealth3"), QStringLiteral("HP 0"));
    data.updateGlobalUnitStatus(robomaster::GlobalUnitStatus{});
    QCOMPARE(text(view, "redControlPointsText"), QStringLiteral("0"));
    QCOMPARE(text(view, "robotHealth3"), QStringLiteral("HP 0"));
    msg.set_total_cp_ally(0xffffffffu);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(text(view, "redControlPointsText"), QStringLiteral("4294967295"));
    data.resetGlobalUnitStatus();
    QCOMPARE(text(view, "redControlPointsText"), QStringLiteral("—"));
    QCOMPARE(text(view, "robotHealth3"), QStringLiteral("HP —"));
  }

  void bluePerspectiveAndIdentityReset() {
    GameData data;
    data.setCurrentRobotId(103);
    QQuickView view;
    load(view, data);
    auto msg = healthPacket(100);
    msg.set_total_cp_ally(12);
    msg.set_total_cp_enemy(34);
    data.updateGlobalUnitStatus(msg);
    QTRY_COMPARE(text(view, "redControlPointsText"), QStringLiteral("34"));
    QCOMPARE(text(view, "blueControlPointsText"), QStringLiteral("12"));
    QCOMPARE(text(view, "robotHealth103"), QStringLiteral("HP 101"));
    QCOMPARE(text(view, "robotHealth3"), QStringLiteral("HP 105"));
    data.setCurrentRobotId(1);
    QCOMPARE(text(view, "blueControlPointsText"), QStringLiteral("—"));
    QCOMPARE(text(view, "robotHealth103"), QStringLiteral("HP —"));
  }

  void serverClockAndPause() {
    GameData data;
    QQuickView view;
    load(view, data);
    QCOMPARE(text(view, "matchClockText"), QStringLiteral("--:--"));
    robomaster::GameStatus msg;
    msg.set_current_stage(4);
    msg.set_current_round(1);
    msg.set_stage_countdown_sec(300);
    data.updateGameStatus(msg);
    QTRY_COMPARE(text(view, "matchClockText"), QStringLiteral("05:00"));
    QCOMPARE(text(view, "matchPhaseText"), QStringLiteral("战斗阶段 · 第 1 局"));
    msg.set_is_paused(true);
    data.updateGameStatus(msg);
    QCOMPARE(text(view, "matchPhaseText"), QStringLiteral("比赛暂停 · 第 1 局"));
    msg.set_stage_countdown_sec(0);
    data.updateGameStatus(msg);
    QCOMPARE(text(view, "matchClockText"), QStringLiteral("00:00"));
    // Do not clamp received times or silently turn legacy simulator 420 into 300.
    msg.set_stage_countdown_sec(420);
    data.updateGameStatus(msg);
    QCOMPARE(text(view, "matchClockText"), QStringLiteral("07:00"));
    QVERIFY(QMetaObject::invokeMethod(&data, "resetMatchStatus"));
    QCOMPARE(text(view, "matchClockText"), QStringLiteral("--:--"));
    data.updateGameStatus(msg);
    data.setCurrentRobotId(3);
    QCOMPARE(text(view, "matchClockText"), QStringLiteral("--:--"));
  }

  void rosterAndLayout_data() {
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::newRow("1080p") << 1920 << 170;
    QTest::newRow("720p") << 1280 << 170;
    QTest::newRow("compact-desktop") << 1280 << 136;
  }
  void rosterAndLayout() {
    QFETCH(int, width);
    QFETCH(int, height);
    GameData data;
    QQuickView view;
    load(view, data, width, height);
    QCOMPARE(view.status(), QQuickView::Ready);
    const QList<int> ids{1, 3, 7, 101, 103, 107};
    for (int id : ids) {
      auto *card = item(view.rootObject(), QStringLiteral("robotCard%1").arg(id));
      QTRY_VERIFY(card);
      const auto bounds = card->mapRectToScene(card->boundingRect());
      QVERIFY2(bounds.left() >= 0 && bounds.right() <= width &&
               bounds.top() >= 0 && bounds.bottom() <= height,
               qPrintable(QStringLiteral("Card %1 outside %2px view").arg(id).arg(width)));
    }
    for (int id : {2, 4, 6, 102, 104, 106})
      QVERIFY(!item(view.rootObject(), QStringLiteral("robotCard%1").arg(id)));
    for (const QString &team : {QStringLiteral("red"), QStringLiteral("blue")}) {
      auto *cp = item(view.rootObject(), team + "ControlPointsText");
      auto *card = item(view.rootObject(), team == "red" ? "robotCard1" : "robotCard101");
      QVERIFY(cp && card);
      QVERIFY2(cp->mapRectToScene(cp->boundingRect()).bottom() <=
                   card->mapRectToScene(card->boundingRect()).top(),
               "Control points are covered by robot cards in the compact desktop HUD");
    }
    auto *phase = item(view.rootObject(), "matchPhaseText");
    QVERIFY(phase);
    QVERIFY(phase->mapRectToScene(phase->boundingRect()).bottom() <= height);
  }

  void testSimulatorMqttFrames() {
#ifndef RM_HAS_MQTT
    QSKIP("MQTT not compiled");
#else
    const QString fixturePath = qEnvironmentVariable("RMUL2027_FIXTURE_PATH");
    if (fixturePath.isEmpty()) QSKIP("Run tools/verify_rmul2027_data.py --hud");
    QFile file(fixturePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
    const int robotId = fixture.value("robot_id").toInt();
    GameData data;
    data.setCurrentRobotId(robotId);
    QQuickView view;
    load(view, data);
    QCOMPARE(view.status(), QQuickView::Ready);
    RM::NetworkManager network(&data);
    QSignalSpy connected(&network, &RM::NetworkManager::mqttConnectCompleted);
    QSignalSpy received(&network, &RM::NetworkManager::dataReceived);
    network.startMqtt(qEnvironmentVariable("RMUL2027_MQTT_URI"), QString::number(robotId));
    QTRY_VERIFY_WITH_TIMEOUT(connected.count() > 0, 10000);
    const auto frames = fixture.value("frames").toArray();
    int index = 0;
    for (const auto &entry : frames) {
      const auto frame = entry.toObject();
      const int before = received.count();
      QSaveFile progress(qEnvironmentVariable("RMUL2027_PROGRESS_PATH") +
                         QStringLiteral(".%1").arg(index));
      QVERIFY(progress.open(QIODevice::WriteOnly));
      progress.write(QJsonDocument(QJsonObject{{"ready", index}}).toJson());
      QVERIFY(progress.commit());
      QTRY_VERIFY_WITH_TIMEOUT(received.count() > before, 5000);
      for (const QString &team : {QStringLiteral("red"), QStringLiteral("blue")}) {
        const auto cp = frame.value(team + "_cp");
        QTRY_COMPARE(text(view, team + "ControlPointsText"),
                     cp.isNull() ? QStringLiteral("—") : QString::number(cp.toInt()));
      }
      const auto hp = frame.value("health").toArray();
      const QList<int> ids{1, 3, 4, 7, 101, 103, 104, 107};
      for (int i : {0, 1, 3, 4, 5, 7})
        QTRY_COMPARE(text(view, QStringLiteral("robotHealth%1").arg(ids[i])),
                     QStringLiteral("HP %1").arg(hp[i].toInt()));
      const auto status = frame.value("match_status").toObject();
      const int seconds = status.value("stage_countdown_sec").toInt();
      QTRY_COMPARE(text(view, "matchClockText"),
                   QStringLiteral("%1:%2").arg(seconds / 60, 2, 10, QChar('0'))
                                         .arg(seconds % 60, 2, 10, QChar('0')));
      QTRY_COMPARE(text(view, "matchPhaseText"),
                   status.value("is_paused").toBool() ? QStringLiteral("比赛暂停 · 第 1 局") :
                                                       QStringLiteral("战斗阶段 · 第 1 局"));
      if (index == 0 || index == 1 || index == 3) {
        const auto path = QFileInfo(fixturePath).dir().filePath(
            QStringLiteral("hud-%1-frame%2.png").arg(robotId).arg(index));
        QTest::qWait(80);
        QVERIFY2(view.grabWindow().save(path), qPrintable(path));
      }
      ++index;
    }
    network.stopMqtt();
    QTRY_COMPARE(text(view, "redControlPointsText"), QStringLiteral("—"));
    QCOMPARE(text(view, "robotHealth1"), QStringLiteral("HP —"));
    QCOMPARE(text(view, "matchClockText"), QStringLiteral("--:--"));
#endif
  }
};
QTEST_MAIN(TestRmul2027Hud)
#include "test_rmul2027_hud.moc"
