#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QtTest>

class TestSettingsPanelShortcuts : public QObject {
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

  void testRmulRosterAndLegacyInternalKeys() {
    QQmlEngine engine;
    QQmlComponent component(
        &engine, QUrl(QStringLiteral("qrc:/qml/SettingsPanel.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> panel(component.create());
    QVERIFY2(panel, qPrintable(component.errorString()));

    const QStringList robotTypes{
        QStringLiteral("R1 - Hero"), QStringLiteral("R3 - Standard"),
        QStringLiteral("R7 - Sentry"), QStringLiteral("B1 - Hero"),
        QStringLiteral("B3 - Standard"), QStringLiteral("B7 - Sentry"),
    };

    for (const QString &robotType : robotTypes) {
      QVariant handled;
      const bool invoked = QMetaObject::invokeMethod(
          panel.data(), "selectRobotByShortcut",
          Q_RETURN_ARG(QVariant, handled),
          Q_ARG(QVariant, QVariant(robotType)));
      QVERIFY2(invoked, qPrintable(robotType));
      QVERIFY2(handled.toBool(), qPrintable(robotType));
      QCOMPARE(panel->property("currentRobotSelection").toString(), robotType);
      QVERIFY(panel->property("robotShortcutPending").toBool());

      QVERIFY(QMetaObject::invokeMethod(panel.data(),
                                        "clearRobotShortcutSelection"));
      QVERIFY(!panel->property("robotShortcutPending").toBool());
    }

    const QString selectionBeforeInvalid =
        panel->property("currentRobotSelection").toString();
    QVariant invalidHandled;
    QVERIFY(QMetaObject::invokeMethod(
        panel.data(), "selectRobotByShortcut",
        Q_RETURN_ARG(QVariant, invalidHandled),
        Q_ARG(QVariant, QVariant(QStringLiteral("B2 - Engineer")))));
    QVERIFY(!invalidHandled.toBool());
    QCOMPARE(panel->property("currentRobotSelection").toString(),
             selectionBeforeInvalid);
    QVERIFY(!panel->property("robotShortcutPending").toBool());
  }

  void testConfirmRequiresDigitSelectionAndUsesLoginSignal() {
    QQmlEngine engine;
    QQmlComponent component(
        &engine, QUrl(QStringLiteral("qrc:/qml/SettingsPanel.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> panel(component.create());
    QVERIFY2(panel, qPrintable(component.errorString()));
    QSignalSpy loginSpy(panel.data(), SIGNAL(loginRequested(QString)));
    QSignalSpy logoutSpy(panel.data(), SIGNAL(logoutRequested()));
    QVERIFY(loginSpy.isValid());
    QVERIFY(logoutSpy.isValid());

    QVariant handledWithoutSelection;
    QVERIFY(QMetaObject::invokeMethod(
        panel.data(), "confirmRobotShortcutLogin",
        Q_RETURN_ARG(QVariant, handledWithoutSelection)));
    QVERIFY(!handledWithoutSelection.toBool());
    QCOMPARE(loginSpy.count(), 0);
    QCOMPARE(logoutSpy.count(), 0);

    QVariant selectionHandled;
    QVERIFY(QMetaObject::invokeMethod(
        panel.data(), "selectRobotByShortcut",
        Q_RETURN_ARG(QVariant, selectionHandled),
        Q_ARG(QVariant, QVariant(QStringLiteral("B7 - Sentry")))));
    QVERIFY(selectionHandled.toBool());

    // 即使已有活动机器人，数字键 + Enter 仍走现有登录槽完成身份切换；
    // 鼠标按钮原有的登录/退出二态行为不在这里改写。
    panel->setProperty("activeRobotLabel", QStringLiteral("R3 - Standard"));
    QVariant loginHandled;
    QVERIFY(QMetaObject::invokeMethod(
        panel.data(), "confirmRobotShortcutLogin",
        Q_RETURN_ARG(QVariant, loginHandled)));
    QVERIFY(loginHandled.toBool());
    QCOMPARE(loginSpy.count(), 1);
    QCOMPARE(loginSpy.takeFirst().at(0).toString(),
             QStringLiteral("B7 - Sentry"));
    QCOMPARE(logoutSpy.count(), 0);
    QVERIFY(!panel->property("robotShortcutPending").toBool());

    QVariant repeatedConfirmHandled;
    QVERIFY(QMetaObject::invokeMethod(
        panel.data(), "confirmRobotShortcutLogin",
        Q_RETURN_ARG(QVariant, repeatedConfirmHandled)));
    QVERIFY(!repeatedConfirmHandled.toBool());
    QCOMPARE(loginSpy.count(), 0);
  }

  void testPendingSelectionClearsWhenPanelCloses() {
    QQmlEngine engine;
    QQmlComponent component(
        &engine, QUrl(QStringLiteral("qrc:/qml/SettingsPanel.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> panel(component.create());
    QVERIFY2(panel, qPrintable(component.errorString()));

    QVariant handled;
    QVERIFY(QMetaObject::invokeMethod(
        panel.data(), "selectRobotByShortcut", Q_RETURN_ARG(QVariant, handled),
        Q_ARG(QVariant, QVariant(QStringLiteral("R1 - Hero")))));
    QVERIFY(handled.toBool());
    QVERIFY(panel->property("robotShortcutPending").toBool());

    panel->setProperty("visible", false);
    QVERIFY(!panel->property("robotShortcutPending").toBool());
  }

  void testPerformanceIsRoleAndPhaseScoped() {
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/qml/SettingsPanel.qml"));
    QScopedPointer<QObject> panel(component.create());
    QVERIFY2(panel, qPrintable(component.errorString()));
    QSignalSpy requests(panel.data(), SIGNAL(performanceSelectionChanged(int,int,int)));
    auto request = [&](int shooter, int chassis) {
      QVariant accepted;
      const bool invoked = QMetaObject::invokeMethod(panel.data(), "requestPerformanceSelection",
          Q_RETURN_ARG(QVariant, accepted), Q_ARG(QVariant, QVariant(shooter)),
          Q_ARG(QVariant, QVariant(chassis)));
      return invoked && accepted.toBool();
    };
    panel->setProperty("matchStage", 1);
    panel->setProperty("selectedRobotType", "R1 - Hero");
    panel->setProperty("activeRobotLabel", "R1 - Hero");
    QVERIFY(!request(1, 1));
    panel->setProperty("selectedRobotType", "R3 - Standard");
    QVERIFY(!request(1, 1)); // Previewing another identity is not a login.
    panel->setProperty("activeRobotLabel", "R3 - Standard");
    QVERIFY(request(2, 1));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.at(0).at(0).toInt(), 2);
    QCOMPARE(requests.at(0).at(1).toInt(), 1);
    QCOMPARE(requests.at(0).at(2).toInt(), 0);
    QVERIFY(request(0, 2)); // Unselected field must remain omitted on wire.
    QVERIFY(!request(3, 1));
    QVERIFY(!request(1, 4));
    QVERIFY(!request(0, 0));
    for (int stage : {0, 4, 5}) {
      panel->setProperty("matchStage", stage);
      QVERIFY(!request(1, 1));
    }
    panel->setProperty("matchStage", 2);
    panel->setProperty("selectedRobotType", "B7 - Sentry");
    panel->setProperty("activeRobotLabel", "B7 - Sentry");
    QVERIFY(!request(1, 1));
    QCOMPARE(requests.count(), 2);
  }

  void testServerFeedbackAndIdentityChangesNeverSendCommands() {
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/qml/SettingsPanel.qml"));
    QScopedPointer<QObject> panel(component.create());
    QVERIFY2(panel, qPrintable(component.errorString()));
    QSignalSpy requests(panel.data(), SIGNAL(performanceSelectionChanged(int,int,int)));
    panel->setProperty("selectedRobotType", "B3 - Standard");
    panel->setProperty("activeRobotLabel", "B3 - Standard");
    panel->setProperty("matchStage", 1);
    panel->setProperty("chassisSelectionText", QStringLiteral("功率优先"));
    panel->setProperty("shooterSelectionText", QStringLiteral("爆发优先"));
    auto *chassis = panel->findChild<QObject *>("chassisSelector");
    auto *shooter = panel->findChild<QObject *>("shooterSelector");
    QVERIFY(chassis); QVERIFY(shooter);
    QCOMPARE(chassis->property("count").toInt(), 3);
    QCOMPARE(shooter->property("count").toInt(), 3);
    QCOMPARE(chassis->property("currentText").toString(), QStringLiteral("功率优先"));
    QCOMPARE(requests.count(), 0);
    QVariant handled;
    QVERIFY(QMetaObject::invokeMethod(panel.data(), "selectRobotByShortcut",
        Q_RETURN_ARG(QVariant, handled), Q_ARG(QVariant, QVariant("R3 - Standard"))));
    QVERIFY(handled.toBool());
    QCOMPARE(chassis->property("currentIndex").toInt(), 0);
    QCOMPARE(shooter->property("currentIndex").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(panel.data(), "selectRobotByShortcut",
        Q_RETURN_ARG(QVariant, handled), Q_ARG(QVariant, QVariant("B3 - Standard"))));
    QCOMPARE(chassis->property("currentText").toString(), QStringLiteral("功率优先"));
    QCOMPARE(shooter->property("currentText").toString(), QStringLiteral("爆发优先"));
    QCOMPARE(requests.count(), 0);
    panel->setProperty("selectedRobotType", "R3 - Standard");
    QCOMPARE(chassis->property("currentIndex").toInt(), 0);
    QCOMPARE(shooter->property("currentIndex").toInt(), 0);
    QCOMPARE(requests.count(), 0);
  }
};

QTEST_MAIN(TestSettingsPanelShortcuts)
#include "test_settings_panel_shortcuts.moc"
