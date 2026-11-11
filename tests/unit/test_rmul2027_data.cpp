#include "core/GameData.h"
#include "network/NetworkManager.h"
#include "robomaster.pb.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QtTest>

class TestRmul2027Data : public QObject {
  Q_OBJECT
private slots:
  void fixedHealthSlots_data() {
    QTest::addColumn<int>("currentId");
    QTest::newRow("red") << 1;
    QTest::newRow("blue") << 103;
  }

  void fixedHealthSlots() {
    QFETCH(int, currentId);
    GameData data;
    data.setCurrentRobotId(currentId);
    robomaster::GlobalUnitStatus msg;
    const QList<int> hp{101, 103, 104, 107, 201, 203, 204, 207};
    const QList<int> red{1, 3, 4, 7}, blue{101, 103, 104, 107};
    const auto ally = currentId >= 100 ? blue : red;
    const auto enemy = currentId >= 100 ? red : blue;
    for (int value : hp) msg.add_robot_health(value);
    for (int value : {11, 13, 14, 17}) msg.add_robot_bullets(value);
    data.updateGlobalUnitStatus(msg);
    for (int i = 0; i < 4; ++i) {
      const auto *a = data.getRobotById(ally[i]);
      const auto *e = data.getRobotById(enemy[i]);
      QCOMPARE(a->currentHP, hp[i]);
      QCOMPARE(e->currentHP, hp[i + 4]);
      QVERIFY(a->hasTabGlobalSnapshot);
      QVERIFY(e->hasTabGlobalSnapshot);
      QCOMPARE(i == 0 ? a->allowedAmmo42mm : a->allowedAmmo17mm, i == 0 ? 11 : (i == 1 ? 13 : (i == 2 ? 14 : 17)));
    }
    QVERIFY(!data.getRobotById(2)->hasTabGlobalSnapshot);
    QVERIFY(!data.getRobotById(6)->hasTabGlobalSnapshot);
  }

  void zeroHealthOverridesConnectedRobot() {
    GameData data;
    robomaster::RobotStaticStatus connected;
    connected.set_robot_id(1);
    connected.set_connection_state(1);
    connected.set_alive_state(1);
    connected.set_max_health(600);
    data.updateRobotStaticStatus(connected);
    robomaster::GlobalUnitStatus msg;
    for (int i = 0; i < 8; ++i) msg.add_robot_health(400);
    data.updateGlobalUnitStatus(msg);
    msg.set_robot_health(0, 0);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(data.getRobotById(1)->currentHP, 0);
    QCOMPARE(data.getRobotById(1)->tabGlobalCurrentHP, 0);
    QVERIFY(data.getRobotById(1)->hasTabGlobalSnapshot);
    QCOMPARE(data.getRobotInfo(1).value("hp").toInt(), 0);
    QCOMPARE(data.getRedRobotsDataQml().first().toMap().value("currentHP").toInt(), 0);
  }

  void missingHealthDoesNotClearSnapshot() {
    GameData data;
    robomaster::GlobalUnitStatus msg;
    for (int i = 0; i < 8; ++i) msg.add_robot_health(123);
    data.updateGlobalUnitStatus(msg);
    data.updateGlobalUnitStatus(robomaster::GlobalUnitStatus{});
    QVERIFY(data.getRobotById(1)->hasTabGlobalSnapshot);
    QCOMPARE(data.getRobotById(1)->tabGlobalCurrentHP, 123);
    QCOMPARE(data.redRobotTotalHP(), 492);
  }

  void controlPointsPresenceAndZero_data() { fixedHealthSlots_data(); }

  void controlPointsPresenceAndZero() {
    QFETCH(int, currentId);
    GameData data;
    data.setCurrentRobotId(currentId);
    QSignalSpy spy(&data, &GameData::controlPointsChanged);
    QVERIFY(data.property("hasRedControlPoints").isValid());
    data.updateGlobalUnitStatus(robomaster::GlobalUnitStatus{});
    QVERIFY(!data.hasRedControlPoints());
    QVERIFY(!data.hasBlueControlPoints());
    QCOMPARE(spy.count(), 0);

    // 与 Python 相同的固定 wire 金样：己方0、对方600。
    const auto payload = QByteArray::fromHex("78008001d804");
    robomaster::GlobalUnitStatus msg;
    QVERIFY(msg.ParseFromArray(payload.data(), payload.size()));
    QVERIFY(msg.has_total_cp_ally());
    QVERIFY(msg.has_total_cp_enemy());
    data.updateGlobalUnitStatus(msg);
    QVERIFY(data.hasRedControlPoints());
    QVERIFY(data.hasBlueControlPoints());
    QCOMPARE(data.redControlPoints(), currentId >= 100 ? 600u : 0u);
    QCOMPARE(data.blueControlPoints(), currentId >= 100 ? 0u : 600u);
    QCOMPARE(spy.count(), 1);
    data.updateGlobalUnitStatus(msg);
    data.updateGlobalUnitStatus(robomaster::GlobalUnitStatus{});
    QCOMPARE(spy.count(), 1);
    QCOMPARE(data.redControlPoints(), currentId >= 100 ? 600u : 0u);
    QCOMPARE(data.blueControlPoints(), currentId >= 100 ? 0u : 600u);

    // 一侧显式归零，另一侧缺失，不得互相覆盖。
    msg.Clear();
    msg.set_total_cp_enemy(0);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(data.redControlPoints(), 0u);
    QCOMPARE(data.blueControlPoints(), 0u);
    QCOMPARE(spy.count(), 2);
    msg.set_total_cp_ally(0xFFFFFFFFu);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(currentId >= 100 ? data.blueControlPoints() : data.redControlPoints(), 0xFFFFFFFFu);
  }

  void oneSidedControlPointsDoNotInventOtherSide() {
    GameData data;
    robomaster::GlobalUnitStatus msg;
    msg.set_total_cp_enemy(0);
    data.updateGlobalUnitStatus(msg);
    QVERIFY(!data.hasRedControlPoints());
    QVERIFY(data.hasBlueControlPoints());
    QCOMPARE(data.blueControlPoints(), 0u);
  }

  void invalidHealthDoesNotShiftOrClearSlots_data() {
    QTest::addColumn<int>("size");
    for (int size : {1, 4, 6, 7, 9, 10, 12})
      QTest::newRow(qPrintable(QString::number(size))) << size;
  }

  void invalidHealthDoesNotShiftOrClearSlots() {
    QFETCH(int, size);
    GameData data;
    robomaster::GlobalUnitStatus msg;
    for (int i = 0; i < 8; ++i) msg.add_robot_health(123);
    data.updateGlobalUnitStatus(msg);
    msg.clear_robot_health();
    for (int i = 0; i < size; ++i) msg.add_robot_health(999);
    msg.set_total_cp_ally(321);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(data.getRobotById(3)->currentHP, 123);
    QVERIFY(data.getRobotById(3)->hasTabGlobalSnapshot);
    QCOMPARE(data.redRobotTotalHP(), 492);
    QCOMPARE(data.redControlPoints(), 321u);
    QVERIFY(!data.getRobotById(2)->hasTabGlobalSnapshot);
    QVERIFY(!data.getRobotById(6)->hasTabGlobalSnapshot);
  }

  void zeroSnapshotAndSparseFields() {
    GameData data;
    robomaster::GlobalUnitStatus msg;
    for (int i = 0; i < 8; ++i) msg.add_robot_health(123);
    msg.set_base_health(5000);
    msg.set_total_damage_ally(900);
    msg.set_total_damage_enemy(800);
    data.updateGlobalUnitStatus(msg);
    msg.Clear();
    msg.set_total_cp_ally(0);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(data.getRedBase().currentHP, 5000);
    QCOMPARE(data.redTotalDamage(), 900);
    QCOMPARE(data.blueTotalDamage(), 800);
    QCOMPARE(data.redRobotTotalHP(), 492);
    for (int i = 0; i < 8; ++i) msg.add_robot_health(0);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(data.redRobotTotalHP(), 0);
    QCOMPARE(data.blueRobotTotalHP(), 0);
    for (int id : {1, 3, 4, 7, 101, 103, 104, 107}) {
      QCOMPARE(data.getRobotById(id)->currentHP, 0);
      QVERIFY(data.getRobotById(id)->hasTabGlobalSnapshot);
    }
    // 超出内部HP表示范围时整组拒绝，不能发生uint16回绕。
    msg.set_robot_health(0, 65536);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(data.getRobotById(1)->currentHP, 0);
    QVERIFY(data.getRobotById(1)->hasTabGlobalSnapshot);
  }

  void bulletsCanUpdateWithoutHealth() {
    GameData data;
    data.setCurrentRobotId(103);
    robomaster::GlobalUnitStatus msg;
    for (int value : {11, 13, 14, 17}) msg.add_robot_bullets(value);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(data.getRobotById(101)->allowedAmmo42mm, 11);
    QCOMPARE(data.getRobotById(103)->allowedAmmo17mm, 13);
    QCOMPARE(data.getRobotById(104)->allowedAmmo17mm, 14);
    QCOMPARE(data.getRobotById(107)->allowedAmmo17mm, 17);
    QVERIFY(!data.getRobotById(103)->hasTabGlobalSnapshot);
    msg.Clear();
    msg.add_robot_bullets(999);
    data.updateGlobalUnitStatus(msg);
    QCOMPARE(data.getRobotById(101)->allowedAmmo42mm, 11);
  }

  void sourceAndRoundChangesInvalidateGlobalSnapshot() {
    GameData data;
    robomaster::GlobalUnitStatus msg;
    msg.set_total_cp_ally(600);
    msg.set_total_cp_enemy(590);
    for (int i = 0; i < 8; ++i) msg.add_robot_health(400);
    data.updateGlobalUnitStatus(msg);
    data.setCurrentRobotId(103);
    QVERIFY(!data.hasRedControlPoints());
    QVERIFY(!data.hasBlueControlPoints());
    QVERIFY(!data.getRobotById(3)->hasTabGlobalSnapshot);
    data.updateGlobalUnitStatus(msg);
    robomaster::GameStatus status;
    status.set_current_round(2);
    status.set_current_stage(1);
    data.updateGameStatus(status);
    QVERIFY(!data.hasRedControlPoints());
    QVERIFY(!data.hasBlueControlPoints());
    data.updateGlobalUnitStatus(msg);
    data.updateGameStatus(status); // 同一回合周期更新不重置。
    QVERIFY(data.hasRedControlPoints());
    data.resetGlobalUnitStatus();
    QVERIFY(!data.hasRedControlPoints());
  }

  void testSimulatorMqttFrames() {
#ifndef RM_HAS_MQTT
    QSKIP("MQTT not compiled");
#else
    const QString fixturePath = qEnvironmentVariable("RMUL2027_FIXTURE_PATH");
    if (fixturePath.isEmpty()) QSKIP("Run tools/verify_rmul2027_data.py for live MQTT validation");
    QFile file(fixturePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
    const int robotId = fixture.value("robot_id").toInt();
    GameData data;
    data.setCurrentRobotId(robotId);
    RM::NetworkManager network(&data);
    QSignalSpy connected(&network, &RM::NetworkManager::mqttConnectCompleted);
    QSignalSpy received(&network, &RM::NetworkManager::dataReceived);
    network.startMqtt(qEnvironmentVariable("RMUL2027_MQTT_URI"), QString::number(robotId));
    QTRY_VERIFY_WITH_TIMEOUT(connected.count() > 0, 10000);
    QVERIFY(network.isMqttConnected());
    const auto frames = fixture.value("frames").toArray();
    int index = 0;
    for (const auto &item : frames) {
      const auto frame = item.toObject();
      const int before = received.count();
      // 每帧一个不可变回执，避免Windows读取旧文件时阻止原子替换。
      QSaveFile progress(qEnvironmentVariable("RMUL2027_PROGRESS_PATH") +
                         QStringLiteral(".%1").arg(index));
      QVERIFY(progress.open(QIODevice::WriteOnly));
      progress.write(QJsonDocument(QJsonObject{{"ready", index}}).toJson());
      QVERIFY(progress.commit());
      QTRY_VERIFY_WITH_TIMEOUT(received.count() > before, 5000);
      const auto red = frame.value("red_cp"), blue = frame.value("blue_cp");
      QCOMPARE(data.hasRedControlPoints(), !red.isNull());
      QCOMPARE(data.hasBlueControlPoints(), !blue.isNull());
      if (!red.isNull()) QCOMPARE(data.redControlPoints(), static_cast<quint32>(red.toDouble()));
      if (!blue.isNull()) QCOMPARE(data.blueControlPoints(), static_cast<quint32>(blue.toDouble()));
      const auto health = frame.value("health").toArray();
      const QList<int> ids{1, 3, 4, 7, 101, 103, 104, 107};
      for (int i = 0; i < ids.size(); ++i) {
        const auto *robot = data.getRobotById(ids[i]);
        QVERIFY(robot->hasTabGlobalSnapshot);
        QCOMPARE(robot->currentHP, health[i].toInt());
        QCOMPARE(robot->tabGlobalCurrentHP, health[i].toInt());
      }
      ++index;
    }
    network.stopMqtt();
    QVERIFY(!data.hasRedControlPoints());
    QVERIFY(!data.hasBlueControlPoints());
#endif
  }
};

QTEST_GUILESS_MAIN(TestRmul2027Data)
#include "test_rmul2027_data.moc"
