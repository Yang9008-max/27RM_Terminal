#include "core/GameData.h"
#include "network/NetworkManager.h"
#include "robomaster.pb.h"
#include <QtTest>

class TestRmul2027Transport : public QObject {
  Q_OBJECT
private slots:
  void latestStateRespectsBudgetAndCancel() {
    QList<QByteArray> sent;
    QList<qint64> times;
    QElapsedTimer clock;
    clock.start();
    RM::ControlSendPolicy policy([&](const QByteArray &payload) {
      times.append(clock.nsecsElapsed()); sent.append(payload); return true;
    });
    QVERIFY(policy.submit("pressed"));
    for (int i = 0; i < 100; ++i) QVERIFY(policy.submit("pressed"));
    QVERIFY(policy.submit("released"));
    QCOMPARE(sent.size(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(sent.size(), 2, 1000);
    QCOMPARE(sent.last(), QByteArray("released"));
    QVERIFY(times.last() - times.first() >= 13000000);
    QVERIFY(policy.submit("anchor")); // Establish a fresh send budget after QTRY's polling delay.
    const auto delivered = sent.size();
    QVERIFY(policy.submit("stale"));
    policy.cancel();
    QTest::qWait(30);
    QCOMPARE(sent.size(), delivered);
    QVERIFY(policy.submit("new session"));
    QCOMPARE(sent.last(), QByteArray("new session"));
  }

  void retryReleaseAndShutdownDrain() {
    int attempts = 0;
    RM::ControlSendPolicy policy([&](const QByteArray &) { return ++attempts > 1; });
    QVERIFY(!policy.submit("release"));
    QTRY_COMPARE_WITH_TIMEOUT(attempts, 2, 1000);
    QVERIFY(policy.submit("final release"));
    policy.finish();
    QCOMPARE(attempts, 3);
  }

  void sparseEconomyAndModules() {
    GameData data;
    QCOMPARE(data.moduleStates().at(3).toInt(), 2);
    robomaster::GlobalLogisticsStatus economy;
    economy.set_remaining_economy(300);
    economy.set_total_economy_obtained(600);
    data.updateGlobalLogisticsStatus(economy);
    economy.Clear(); economy.set_tech_level(2);
    data.updateGlobalLogisticsStatus(economy);
    QCOMPARE(data.redRemainingEconomy(), 300u);
    QCOMPARE(data.redTotalEconomyObtained(), 600ull);
    economy.set_remaining_economy(0);
    data.updateGlobalLogisticsStatus(economy);
    QCOMPARE(data.redRemainingEconomy(), 0u);
    robomaster::RobotModuleStatus module;
    module.set_armor(1);
    data.updateRobotModuleStatus(module);
    QCOMPARE(data.getModuleStatus().size(), 4);
    module.Clear(); module.set_rfid(0);
    data.updateRobotModuleStatus(module);
    int onlineArmor = 0;
    for (const auto &entry : data.getModuleStatus()) {
      const auto name = QString::fromUtf8(entry.moduleName);
      if (name.startsWith(QStringLiteral("装甲")) && entry.isOnline) ++onlineArmor;
      if (name == "RFID") QVERIFY(!entry.isOnline);
    }
    QCOMPARE(onlineArmor, 4);
    QCOMPARE(data.moduleStates().at(4).toInt(), 1);
    QCOMPARE(data.moduleStates().at(6).toInt(), 2);
    QCOMPARE(data.getModuleStatus().size(), 5);
    data.setCurrentRobotId(3);
    QVERIFY(data.getModuleStatus().isEmpty());
  }

  void sparseDynamicPreservesState() {
    GameData data;
    data.setCurrentRobotId(3);
    robomaster::RobotDynamicStatus first;
    first.set_current_health(400);
    first.set_remaining_ammo(90);
    data.updateRobotDynamicStatus(first);
    robomaster::RobotDynamicStatus heat;
    heat.set_current_heat(12);
    data.updateRobotDynamicStatus(heat);
    QCOMPARE(data.getRobotById(3)->currentHP, 400);
    QCOMPARE(data.getRobotById(3)->allowedAmmo17mm, 90);
    data.getRobotDataRef(3).power = 55;
    data.getRobotDataRef(3).currentChassisEnergy = 66;
    heat.set_reserved(999);
    data.updateRobotDynamicStatus(heat);
    QCOMPARE(data.getRobotById(3)->power, 55);
    QCOMPARE(data.getRobotById(3)->currentChassisEnergy, 66);
    heat.set_current_health(0);
    heat.set_remaining_ammo(0);
    data.updateRobotDynamicStatus(heat);
    QCOMPARE(data.getRobotById(3)->currentHP, 0);
    QCOMPARE(data.getRobotById(3)->allowedAmmo17mm, 0);
  }

  void sparseStaticPreservesState() {
    GameData data;
    data.setCurrentRobotId(3);
    robomaster::RobotStaticStatus first;
    first.set_robot_id(3);
    first.set_max_health(500);
    first.set_level(2);
    first.set_connection_state(1);
    first.set_performance_system_shooter(1);
    data.updateRobotStaticStatus(first);
    robomaster::RobotPerformanceSelectionSync newerSelection;
    newerSelection.set_shooter(2);
    data.updateRobotPerformanceSelectionSync(newerSelection);
    robomaster::RobotStaticStatus update;
    update.set_robot_id(3);
    update.set_max_heat(100);
    data.updateRobotStaticStatus(update);
    QCOMPARE(data.getRobotById(3)->maxHP, 500);
    QCOMPARE(data.getRobotById(3)->level, 2);
    QCOMPARE(data.getRobotById(3)->shooterPerformanceSelection, 2);
    QVERIFY(data.getRobotById(3)->isClientConnected);
    update.Clear(); update.set_field_state(1);
    data.updateRobotStaticStatus(update);
    update.Clear(); update.set_alive_state(1);
    data.updateRobotStaticStatus(update);
    QCOMPARE(data.getRobotById(3)->status, RobotStatus::OFFLINE);
  }

  void releaseHasExplicitZeroFields() {
    robomaster::KeyboardMouseControl release;
    const auto *descriptor = release.GetDescriptor();
    auto *reflection = release.GetReflection();
    for (int i = 0; i < descriptor->field_count(); ++i) {
      const auto *field = descriptor->field(i);
      QVERIFY2(field->has_presence(), std::string(field->name()).c_str());
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT32)
        reflection->SetUInt32(&release, field, 0);
      else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_BOOL)
        reflection->SetBool(&release, field, false);
      else
        reflection->SetInt32(&release, field, 0);
    }
    const auto wire = release.SerializeAsString();
    QVERIFY(!wire.empty());
    robomaster::KeyboardMouseControl decoded;
    QVERIFY(decoded.ParseFromString(wire));
    for (int i = 0; i < descriptor->field_count(); ++i)
      QVERIFY(reflection->HasField(decoded, descriptor->field(i)));
  }

  void liveMqttBoundariesAndRouting() {
#ifndef RM_HAS_MQTT
    QSKIP("MQTT unavailable");
#else
    const QString uri = qEnvironmentVariable("RMUL_TRANSPORT_MQTT_URI");
    if (uri.isEmpty()) QSKIP("Run tools/verify_rmul2027_transport.py for private broker verification");
    MqttManager peer;
    QSignalSpy peerReady(&peer, &MqttManager::connectCompleted);
    peer.connectToBrokerAsync(uri, "transport-test-peer");
    QTRY_VERIFY_WITH_TIMEOUT(peerReady.count(), 10000);
    QVERIFY(peer.isConnected());
    for (const auto *topic : {"KeyboardMouseControl", "CustomControl", "MapClickCmd"})
      QVERIFY(peer.subscribe(topic));
    QSignalSpy received(&peer, &MqttManager::messageReceived);
    GameData data;
    RM::NetworkManager network(&data);
    QSignalSpy ready(&network, &RM::NetworkManager::mqttConnectCompleted);
    network.startMqtt(uri, "1");
    QTRY_VERIFY_WITH_TIMEOUT(ready.count(), 10000);
    QVERIFY(network.isMqttConnected());
    QVERIFY(network.sendKeyboardMouseControl(0, 0, 0, true, false, false, 1));
    QVERIFY(network.sendKeyboardMouseControl(0, 0, 0, false, false, false, 0));
    network.sendData(QByteArray(60, 'x'), QHostAddress::LocalHost, 0);
    network.sendData(QByteArray(61, 'x'), QHostAddress::LocalHost, 0);
    network.sendMapClickInfo(3, 2, 3, 3, 0, 0, false);
    QTRY_COMPARE_WITH_TIMEOUT(received.count(), 4, 5000);
    int keyboard = 0, custom = 0, map = 0;
    for (const auto &event : received) {
      const auto topic = event.at(0).toString();
      const auto payload = event.at(1).toByteArray();
      if (topic == "KeyboardMouseControl") {
        robomaster::KeyboardMouseControl msg;
        QVERIFY(msg.ParseFromString(payload.toStdString()));
        QVERIFY(msg.has_keyboard_value()); QVERIFY(msg.has_left_button_down());
        if (++keyboard == 2) { QCOMPARE(msg.keyboard_value(), 0u); QVERIFY(!msg.left_button_down()); }
      } else if (topic == "CustomControl") {
        robomaster::CustomControl msg;
        QVERIFY(msg.ParseFromString(payload.toStdString()));
        QCOMPARE(msg.data().size(), size_t(60)); ++custom;
      } else if (topic == "MapClickCmd") {
        robomaster::MapClickCmd msg;
        QVERIFY(msg.ParseFromString(payload.toStdString()));
        QCOMPARE(msg.robot_id().size(), size_t(7));
        QCOMPARE(int(msg.robot_id()[0]), 3); ++map;
        QCOMPARE(msg.mode(), 3u); // Field #3 is marker kind per the numbered table.
        QCOMPARE(msg.type(), 1u); // Field #6 selects a map coordinate marker.
      }
    }
    QCOMPARE(keyboard, 2); QCOMPARE(custom, 1); QCOMPARE(map, 1);
    QSignalSpy raw(&network, &RM::NetworkManager::customDataPayloadReceived);
    QSignalSpy video(&network, &RM::NetworkManager::customVideoPayloadReceived);
    QSignalSpy markers(&network, &RM::NetworkManager::mapClickInfoReceived);
    data.getRobotDataRef(1).fricEnabled = true;
    robomaster::CustomByteBlock block;
    block.set_data(std::string(28, '\0'));
    qunsetenv("RM_CUSTOM_DATA_FORMAT");
    QVERIFY(peer.publish("CustomByteBlock", QByteArray::fromStdString(block.SerializeAsString())));
    QTRY_COMPARE(raw.count(), 1);
    QCOMPARE(video.count(), 0);
    QVERIFY(data.getCurrentRobot()->fricEnabled);
    QCOMPARE(raw.at(0).at(0).toByteArray(), QByteArray(28, '\0'));
    qputenv("RM_CUSTOM_DATA_FORMAT", "h264");
    QVERIFY(peer.publish("CustomByteBlock", QByteArray::fromStdString(block.SerializeAsString())));
    QTRY_COMPARE(video.count(), 1);
    block.set_data(std::string(301, 'x'));
    QVERIFY(peer.publish("CustomByteBlock", QByteArray::fromStdString(block.SerializeAsString())));
    robomaster::MapClickInfo info;
    info.set_robot_id(std::string(7, '\0')); info.set_map_x(2); info.set_map_y(3);
    QVERIFY(peer.publish("MapClickInfo", QByteArray::fromStdString(info.SerializeAsString())));
    QTRY_COMPARE(markers.count(), 1);
    QTest::qWait(30);
    QCOMPARE(raw.count(), 2);
    QCOMPARE(received.count(), 4); // Downlink must never cause command echo.
    robomaster::GlobalLogisticsStatus economy;
    economy.set_remaining_economy(300);
    QVERIFY(peer.publish("GlobalLogisticsStatus", QByteArray::fromStdString(economy.SerializeAsString())));
    QTRY_COMPARE(data.redEconomy(), 300);
    economy.Clear(); economy.set_tech_level(2);
    QVERIFY(peer.publish("GlobalLogisticsStatus", QByteArray::fromStdString(economy.SerializeAsString())));
    QTRY_COMPARE(data.redTechLevel(), 2u);
    QCOMPARE(data.redEconomy(), 300);
    received.clear();
    QVERIFY(network.sendKeyboardMouseControl(0, 0, 0, true, false, false, 1));
    QVERIFY(network.sendKeyboardMouseControl(0, 0, 0, true, false, false, 2));
    network.stopMqtt(); // Drain a neutral frame before disconnect, discard queued actions.
    QTRY_COMPARE_WITH_TIMEOUT(received.count(), 2, 5000);
    robomaster::KeyboardMouseControl finalFrame;
    QVERIFY(finalFrame.ParseFromString(received.last().at(1).toByteArray().toStdString()));
    QVERIFY(finalFrame.has_keyboard_value());
    QCOMPARE(finalFrame.keyboard_value(), 0u);
    QVERIFY(!finalFrame.left_button_down());
    network.startMqtt(uri, "1");
    QTRY_VERIFY_WITH_TIMEOUT(network.isMqttConnected(), 10000);
    QTest::qWait(30);
    QCOMPARE(received.count(), 2);
    network.stopMqtt();
    peer.disconnect();
    qunsetenv("RM_CUSTOM_DATA_FORMAT");
#endif
  }
};
QTEST_MAIN(TestRmul2027Transport)
#include "test_rmul2027_transport.moc"
