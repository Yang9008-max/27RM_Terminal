#include <QtTest>
#include "core/GameData.h"
#include "robomaster.pb.h"
#include "widgets/GameResultWidget.h"
#include "network/NetworkManager.h"
#include "ui/MainWindow.h"
#include <QKeyEvent>
#include <QLabel>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QDir>

class TestRmul2027Settlement : public QObject {
  Q_OBJECT
private slots:
  void initTestCase() {
    Q_INIT_RESOURCE(qml);
    Q_INIT_RESOURCE(resources);
    qputenv("RM_DISABLE_AUDIO", "1");
  }
  void mainWindowAutoShowsAndHonorsDismissal() {
    RM::MainWindow window;
    QKeyEvent operation(QEvent::KeyPress, Qt::Key_T, Qt::ControlModifier);
    QApplication::sendEvent(window.centralWidget(), &operation);
    auto *data = window.findChild<GameData *>();
    auto *panel = window.findChild<GameResultWidget *>();
    QVERIFY(data && panel);
    robomaster::GameStatus status;
    status.set_current_stage(4);
    status.set_current_round(1);
    data->updateGameStatus(status);
    status.set_current_stage(5);
    status.set_game_result(1);
    status.set_end_reason(2);
    data->updateGameStatus(status);
    QVERIFY(!panel->isHidden());
    QKeyEvent tactical(QEvent::KeyPress, Qt::Key_T, Qt::ControlModifier);
    QApplication::sendEvent(window.centralWidget(), &tactical);
    QVERIFY(!panel->isHidden());
    QKeyEvent close(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&window, &close);
    QVERIFY(panel->isHidden());
    QKeyEvent backToOperation(QEvent::KeyPress, Qt::Key_T, Qt::ControlModifier);
    QApplication::sendEvent(window.centralWidget(), &backToOperation);
    QVERIFY(panel->isHidden());
    data->updateGameStatus(status);
    QVERIFY(panel->isHidden());
    status.set_current_round(2);
    status.set_game_result(2);
    data->updateGameStatus(status);
    QVERIFY(!panel->isHidden());
    data->resetMatchStatus();
    QVERIFY(panel->isHidden());
    data->updateGameStatus(status);
    QVERIFY(!panel->isHidden());
  }
  void officialResultOverridesScore() {
    GameData data;
    robomaster::GameStatus status;
    status.set_current_stage(5);
    status.set_current_round(1);
    status.set_red_score(0);
    status.set_blue_score(9);
    status.set_game_result(1);
    status.set_end_reason(2);
    status.set_stage_elapsed_sec(300);
    data.updateGameStatus(status);
    QCOMPARE(data.determineWinner(), quint8(1));
    QCOMPARE(data.getFormattedStageElapsedTime(), QStringLiteral("05:00"));
  }
  void missingIsNotDrawAndLateResultIsAccepted() {
    GameData data;
    robomaster::GameStatus status;
    status.set_current_stage(5);
    status.set_red_score(9);
    data.updateGameStatus(status);
    QCOMPARE(data.determineWinner(), quint8(255));
    status.set_game_result(0);
    status.set_end_reason(5);
    data.updateGameStatus(status);
    QCOMPARE(data.determineWinner(), quint8(0));
  }
  void eventCannotPreemptOfficialStage() {
    GameData data;
    robomaster::GameStatus status;
    status.set_current_stage(4);
    status.set_game_result(2);
    data.updateGameStatus(status);
    data.processGameResult(2);
    QCOMPARE(data.determineWinner(), quint8(255));
    QVERIFY(data.isGameActive());
  }
  void sparseInvalidAndSessionReset() {
    GameData data;
    robomaster::GameStatus status;
    status.set_current_stage(5);
    status.set_current_round(1);
    status.set_game_result(0);
    status.set_end_reason(5);
    status.set_stage_elapsed_sec(300);
    data.updateGameStatus(status);
    data.updateGameStatus(robomaster::GameStatus{});
    QCOMPARE(data.determineWinner(), quint8(0));
    QCOMPARE(data.settlementSnapshot().endReason, quint8(5));
    status.set_game_result(255);
    status.set_end_reason(255);
    data.updateGameStatus(status);
    QCOMPARE(data.determineWinner(), quint8(255));
    status.set_game_result(2);
    data.updateGameStatus(status);
    data.resetMatchStatus();
    QCOMPARE(data.determineWinner(), quint8(255));
    QVERIFY(!data.settlementSnapshot().inSettlement);
    QVERIFY(!data.settlementSnapshot().elapsed);
    status.clear_game_result();
    data.updateGameStatus(status);
    QCOMPARE(data.determineWinner(), quint8(255));
  }
  void newRoundAndIdentityCannotInheritResult() {
    GameData data;
    robomaster::GameStatus status;
    status.set_current_stage(5);
    status.set_current_round(1);
    status.set_game_result(1);
    status.set_end_reason(2);
    status.set_stage_elapsed_sec(300);
    data.updateGameStatus(status);
    status.set_current_round(2);
    status.clear_stage_elapsed_sec();
    status.clear_game_result();
    status.clear_end_reason();
    data.updateGameStatus(status);
    QCOMPARE(data.determineWinner(), quint8(255));
    QVERIFY(!data.settlementSnapshot().elapsed);
    status.set_game_result(2);
    data.updateGameStatus(status);
    data.setCurrentRobotId(103);
    QCOMPARE(data.determineWinner(), quint8(255));
  }
  void projectionUsesOfficialDamageAndThreeRobotHealth() {
    for (int id : {1, 103}) {
      GameData data;
      data.setCurrentRobotId(id);
      robomaster::GlobalUnitStatus unit;
      unit.set_total_cp_ally(0);
      unit.set_total_cp_enemy(0xffffffffu);
      unit.set_total_damage_ally(0xffffffffu);
      const QList<int> hp{500, 123, 600, 300, 450, 350, 0, 250};
      for (int value : hp) unit.add_robot_health(value);
      data.updateGlobalUnitStatus(unit);
      auto snapshot = data.settlementSnapshot();
      const auto &ally = id == 1 ? snapshot.redHealth : snapshot.blueHealth;
      QCOMPARE(*ally[2], quint32(300));
      QCOMPARE(*(id == 1 ? snapshot.redDamage : snapshot.blueDamage), quint32(0xffffffffu));
      QVERIFY(!(id == 1 ? snapshot.blueDamage : snapshot.redDamage));
      data.resetGlobalUnitStatus();
      snapshot = data.settlementSnapshot();
      QVERIFY(!snapshot.redDamage && !snapshot.blueDamage);
      QVERIFY(!snapshot.redControlPoints && !snapshot.redHealth[0]);
    }
  }
  void testSimulatorMqttFrames() {
#ifndef RM_HAS_MQTT
    QSKIP("MQTT not compiled");
#else
    const QString fixturePath = qEnvironmentVariable("RMUL2027_FIXTURE_PATH");
    if (fixturePath.isEmpty()) QSKIP("Run tools/verify_rmul2027_data.py --settlement");
    QFile file(fixturePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
    const int robotId = fixture["robot_id"].toInt();
    GameData data;
    data.setCurrentRobotId(robotId);
    GameResultWidget widget;
    widget.setPlayVictoryOnShow(false);
    widget.show();
    connect(&data, &GameData::settlementChanged, &widget, [&] { widget.setSnapshot(data.settlementSnapshot()); });
    RM::NetworkManager network(&data);
    QSignalSpy connected(&network, &RM::NetworkManager::mqttConnectCompleted);
    network.startMqtt(qEnvironmentVariable("RMUL2027_MQTT_URI"), QString::number(robotId));
    QTRY_VERIFY_WITH_TIMEOUT(connected.count() > 0, 10000);
    int index = 0;
    for (auto value : fixture["frames"].toArray()) {
      const auto frame = value.toObject();
      QSaveFile progress(qEnvironmentVariable("RMUL2027_PROGRESS_PATH") + QStringLiteral(".%1").arg(index));
      QVERIFY(progress.open(QIODevice::WriteOnly));
      progress.write(QJsonDocument(QJsonObject{{"ready", index}}).toJson());
      QVERIFY(progress.commit());
      const auto expected = frame["expected"].toObject();
      QTRY_COMPARE(data.determineWinner(), quint8(expected["winner"].toInt()));
      auto label = [&](const char *name) { return widget.findChild<QLabel *>(name)->text(); };
      QTRY_COMPARE(label("result"), expected["title"].toString());
      QTRY_COMPARE(label("red_controlPoints"), expected["red_cp"].toString());
      QTRY_COMPARE(label("blue_controlPoints"), expected["blue_cp"].toString());
      QTRY_COMPARE(label("red_healthTotal"), expected["red_hp"].toString());
      QTRY_COMPARE(label("blue_healthTotal"), expected["blue_hp"].toString());
      QTRY_COMPARE(data.settlementSnapshot().endReason, quint8(expected["reason"].toInt()));
      QTRY_VERIFY(label("series").contains(expected["elapsed"].toString()));
      // Each frame changes round or winner, so the acknowledgement cannot pass
      // against the previous frame's coincidentally equal statistic.
      QTRY_COMPARE(data.currentRound(), frame["match_status"].toObject()["gameStatus"].toObject()["current_round"].toInt());
      QVERIFY(widget.grab().save(QFileInfo(fixturePath).dir().filePath(QStringLiteral("settlement-%1-%2.png").arg(robotId).arg(index))));
      ++index;
    }
    network.stopMqtt();
    QTRY_COMPARE(data.determineWinner(), quint8(255));
    QTRY_COMPARE(widget.findChild<QLabel *>("result")->text(), QStringLiteral("等待裁判结果"));
#endif
  }
};
QTEST_MAIN(TestRmul2027Settlement)
#include "test_rmul2027_settlement.moc"
