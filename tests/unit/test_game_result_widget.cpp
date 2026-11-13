#include <QtTest>
#include <QLabel>

#include "widgets/GameResultWidget.h"

class TestGameResultWidget : public QObject {
  Q_OBJECT

private slots:
  void testRmulResultsAndUnknownValues() {
    GameResultWidget widget;
    auto text = [&](const char *name) { return widget.findChild<QLabel *>(name)->text(); };
    RmulSettlementSnapshot s;
    s.inSettlement = true;
    widget.setSnapshot(s);
    QCOMPARE(text("result"), QStringLiteral("等待裁判结果"));
    QCOMPARE(text("red_controlPoints"), QStringLiteral("—"));
    QCOMPARE(text("red_healthTotal"), QStringLiteral("—"));
    s.winner = 1; s.endReason = 2;
    s.redControlPoints = 600; s.blueControlPoints = 590;
    s.redHealth = {500, 123, 300}; s.blueHealth = {450, 350, 250};
    s.redDamage = 0xffffffffu; s.elapsed = 300;
    widget.setSnapshot(s);
    QCOMPARE(text("result"), QStringLiteral("红方获胜"));
    QCOMPARE(text("red_healthTotal"), QStringLiteral("923"));
    QCOMPARE(text("blue_healthTotal"), QStringLiteral("1050"));
    QCOMPARE(text("red_damage"), QStringLiteral("4294967295"));
    QVERIFY(text("series").contains("05:00"));
    s.redControlPoints = 0; s.redHealth[0] = 0; s.winner = 2;
    widget.setSnapshot(s);
    QCOMPARE(text("result"), QStringLiteral("蓝方获胜"));
    QCOMPARE(text("red_controlPoints"), QStringLiteral("0"));
    QCOMPARE(text("red_hp0"), QStringLiteral("HP 0"));
    s.winner = 0; s.endReason = 5;
    widget.setSnapshot(s);
    QCOMPARE(text("result"), QStringLiteral("本局平局"));
    s.endReason = 6;
    widget.setSnapshot(s);
    QCOMPARE(text("result"), QStringLiteral("比赛异常终止"));
    for (auto *label : widget.findChildren<QLabel *>()) {
      QVERIFY(!label->text().contains(QStringLiteral("前哨")));
      QVERIFY(!label->text().contains(QStringLiteral("能量机关")));
    }
  }
  void testStatisticsRefreshDoesNotAnnounceAgain() {
    GameResultWidget widget;
    widget.setPlayVictoryOnShow(false);
    QSignalSpy announcements(&widget, &GameResultWidget::resultAnnounced);
    RmulSettlementSnapshot s;
    s.inSettlement = true;
    widget.setSnapshot(s); widget.show();
    QCOMPARE(announcements.count(), 0);
    s.winner = 1; s.endReason = 2;
    widget.setSnapshot(s);
    QCOMPARE(announcements.count(), 1);
    s.redControlPoints = 600;
    widget.setSnapshot(s); widget.setSnapshot(s);
    QCOMPARE(announcements.count(), 1);
    s.endReason = 255;
    widget.setSnapshot(s);
    s.endReason = 2;
    widget.setSnapshot(s);
    QCOMPARE(announcements.count(), 1);
  }
  void testLabelsStayWithinCompactViewport() {
    GameResultWidget widget;
    widget.setResolutionViewport(QSize(1280,800));
    widget.show();
    for (auto *label : widget.findChildren<QLabel *>())
      QVERIFY(widget.rect().contains(label->geometry()));
  }
  void testVictorySoundEnabledByDefault() {
    GameResultWidget widget;
    QVERIFY(widget.playVictoryOnShow());
  }

  void testVictorySoundCanBeDisabledExplicitly() {
    GameResultWidget widget;
    widget.setPlayVictoryOnShow(false);
    QVERIFY(!widget.playVictoryOnShow());
  }

  void testResolutionScalingUsesArbitraryViewport() {
    GameResultWidget widget;

    widget.setResolutionViewport(QSize(1600, 1000));

    QCOMPARE(widget.size(), QSize(1000, 667));
    QCOMPARE(widget.resolutionViewport(), QSize(1600, 1000));
  }

  void testResolutionScalingUsesLimitingDimension() {
    GameResultWidget widget;

    widget.setResolutionViewport(QSize(3440, 1440));

    QCOMPARE(widget.size(), QSize(1600, 1067));
  }

  void testInvalidResolutionViewportFallsBackToBaseline() {
    GameResultWidget widget;
    widget.setResolutionViewport(QSize(2560, 1440));

    widget.setResolutionViewport(QSize());

    QCOMPARE(widget.size(), GameResultWidget::designSize());
    QCOMPARE(widget.resolutionViewport(), QSize(1920, 1080));
  }
};

QTEST_MAIN(TestGameResultWidget)
#include "test_game_result_widget.moc"
