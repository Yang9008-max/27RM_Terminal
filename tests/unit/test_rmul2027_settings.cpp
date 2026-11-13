#include "core/GameData.h"
#include "core/RmulSettingsPolicy.h"
#include "network/MqttManager.h"
#include "network/NetworkManager.h"
#include "robomaster.pb.h"
#include <QSignalSpy>
#include <QtTest>

class TestRmul2027Settings : public QObject {
  Q_OBJECT
private slots:
  void rolePhaseAndEnumGuards() {
    using namespace RM::RmulSettingsPolicy;
    for (int id : {1,3,7,101,103,107}) QVERIFY(isRosterRobot(id));
    for (int id : {0,2,4,5,6,102,104,106,255}) QVERIFY(!isRosterRobot(id));
    for (int id : {3,103}) {
      for (int stage : {1,2,3}) QVERIFY(canSelectPerformance(id,true,stage));
      for (int stage : {0,4,5}) QVERIFY(!canSelectPerformance(id,true,stage));
      QVERIFY(!canSelectPerformance(id,false,1));
    }
    for (int id : {1,7,101,107}) QVERIFY(!canSelectPerformance(id,true,1));
    QVERIFY(validPerformanceSelection(0,2,0));
    QVERIFY(validPerformanceSelection(2,0,0));
    QVERIFY(!validPerformanceSelection(3,1,0));
    QVERIFY(!validPerformanceSelection(1,4,0));
    QVERIFY(!validPerformanceSelection(1,1,1));
    QVERIFY(!validPerformanceSelection(0,0,0));
  }
  void feedbackPresenceInvalidEnumsAndSessionReset() {
    for (int id : {3,103}) {
      GameData data;
      data.setCurrentRobotId(id);
      QVERIFY(data.needsInfantryPerformanceSelection());
      robomaster::RobotPerformanceSelectionSync sync;
      sync.set_shooter(2); sync.set_chassis(1);
      data.updateRobotPerformanceSelectionSync(sync);
      QVERIFY(!data.needsInfantryPerformanceSelection());
      QCOMPARE(data.getCurrentRobot()->shooterPerformanceSelection,2);
      sync.clear_shooter(); sync.set_chassis(2);
      data.updateRobotPerformanceSelectionSync(sync);
      QCOMPARE(data.getCurrentRobot()->shooterPerformanceSelection,2);
      QCOMPARE(data.getCurrentRobot()->chassisPerformanceSelection,2);
      robomaster::RobotStaticStatus status;
      status.set_robot_id(id);
      data.updateRobotStaticStatus(status);
      QCOMPARE(data.getCurrentRobot()->shooterPerformanceSelection,2);
      sync.set_shooter(257); // Must not narrow into a valid value of 1.
      data.updateRobotPerformanceSelectionSync(sync);
      QCOMPARE(data.getCurrentRobot()->shooterPerformanceSelection,0);
      QVERIFY(data.needsInfantryPerformanceSelection());
      data.resetMatchStatus();
      QCOMPARE(data.getCurrentRobot()->chassisPerformanceSelection,0);
    }
    GameData heavy;
    QVERIFY(!heavy.needsInfantryPerformanceSelection());
    robomaster::RobotPerformanceSelectionSync sync;
    sync.set_shooter(2);
    heavy.updateRobotPerformanceSelectionSync(sync);
    QCOMPARE(heavy.getCurrentRobot()->shooterPerformanceSelection,0);
    heavy.setCurrentRobotId(7);
    QVERIFY(!heavy.needsInfantryPerformanceSelection());
  }
  void liveMqttCommandFieldsAndSparseFeedback() {
#ifndef RM_HAS_MQTT
    QSKIP("MQTT unavailable");
#else
    const QString uri = qEnvironmentVariable("RMUL_SETTINGS_MQTT_URI");
    if (uri.isEmpty()) QSKIP("Run tools/verify_rmul2027_settings.py for actual broker verification");
    MqttManager peer;
    QSignalSpy peerReady(&peer,&MqttManager::connectCompleted);
    peer.connectToBrokerAsync(uri,"rmul-settings-test-peer");
    QTRY_VERIFY_WITH_TIMEOUT(peerReady.count(),10000);
    QVERIFY(peer.isConnected());
    QVERIFY(peer.subscribe("RobotPerformanceSelectionCommand"));
    QSignalSpy received(&peer,&MqttManager::messageReceived);
    GameData data;
    data.setCurrentRobotId(3);
    RM::NetworkManager network(&data);
    QSignalSpy ready(&network,&RM::NetworkManager::mqttConnectCompleted);
    network.startMqtt(uri,"3");
    QTRY_VERIFY_WITH_TIMEOUT(ready.count(),10000);
    QVERIFY(ready.at(0).at(0).toBool());
    QSignalSpy sent(&network,&RM::NetworkManager::dataSent);
    network.sendRobotPerformanceSelection(1,0,0);
    network.sendRobotPerformanceSelection(0,2,0);
    network.sendRobotPerformanceSelection(2,1,0);
    network.sendRobotPerformanceSelection(3,1,0);
    network.sendRobotPerformanceSelection(1,4,0);
    network.sendRobotPerformanceSelection(1,1,1);
    network.sendRobotPerformanceSelection(0,0,0);
    QCOMPARE(sent.count(),3);
    QTRY_COMPARE(received.count(),3);
    for (int i=0;i<3;++i) {
      robomaster::RobotPerformanceSelectionCommand command;
      QVERIFY(command.ParseFromString(received.at(i).at(1).toByteArray().toStdString()));
      QVERIFY(!command.has_sentry_control());
      QCOMPARE(command.has_shooter(),i!=1);
      QCOMPARE(command.has_chassis(),i!=0);
    }
    robomaster::RobotPerformanceSelectionSync feedback;
    feedback.set_shooter(2); feedback.set_chassis(1);
    QVERIFY(peer.publish("RobotPerformanceSelectionSync",QByteArray::fromStdString(feedback.SerializeAsString())));
    QTRY_COMPARE(data.getCurrentRobot()->shooterPerformanceSelection,2);
    QTRY_COMPARE(data.getCurrentRobot()->chassisPerformanceSelection,1);
    feedback.clear_shooter(); feedback.set_chassis(2);
    QVERIFY(peer.publish("RobotPerformanceSelectionSync",QByteArray::fromStdString(feedback.SerializeAsString())));
    QTRY_COMPARE(data.getCurrentRobot()->chassisPerformanceSelection,2);
    QCOMPARE(data.getCurrentRobot()->shooterPerformanceSelection,2);
    QCOMPARE(sent.count(),3); // Feedback cannot echo another outgoing command.
    network.stopMqtt();
    peer.disconnect();
#endif
  }
};
QTEST_MAIN(TestRmul2027Settings)
#include "test_rmul2027_settings.moc"
