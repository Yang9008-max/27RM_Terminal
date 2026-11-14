#include "widgets/VideoBackgroundWidget.h"
#include <QtTest>

class TestVideoBackgroundWidget : public QObject {
  Q_OBJECT

private slots:
  void testRaisingCapReschedulesPendingFrameAndStopClearsIt() {
    RM::VideoBackgroundWidget widget;
    widget.resize(64, 64);
    widget.playUrl("udp://127.0.0.1:0");
    widget.setTargetFps(1);
    widget.show();
    QImage image(16, 16, QImage::Format_RGB32);
    image.fill(Qt::red);
    QSignalSpy frames(&widget, &RM::VideoBackgroundWidget::frameUpdated);
    QVERIFY(QMetaObject::invokeMethod(&widget, "onImageReceived", Q_ARG(QImage, image)));
    QTRY_COMPARE_WITH_TIMEOUT(frames.count(), 1, 200);
    image.fill(Qt::blue);
    QVERIFY(QMetaObject::invokeMethod(&widget, "onImageReceived", Q_ARG(QImage, image)));
    QTest::qWait(25);
    QCOMPARE(frames.count(), 1);
    widget.setTargetFps(120);
    QTRY_COMPARE_WITH_TIMEOUT(frames.count(), 2, 150);
    QCOMPARE(qvariant_cast<QImage>(frames.last().at(0)), image);
    widget.setTargetFps(1);
    image.fill(Qt::green);
    QVERIFY(QMetaObject::invokeMethod(&widget, "onImageReceived", Q_ARG(QImage, image)));
    widget.stopVideo();
    QTest::qWait(30);
    QCOMPARE(frames.count(), 2);
  }

  void testFrameCapSurvivesExternalRepaints() {
    RM::VideoBackgroundWidget widget;
    widget.resize(64, 64);
    widget.setTargetFps(10);
    widget.playUrl("udp://127.0.0.1:0");
    widget.show();
    QImage image(16, 16, QImage::Format_RGB32);
    QSignalSpy frames(&widget, &RM::VideoBackgroundWidget::frameUpdated);
    QElapsedTimer elapsed;
    elapsed.start();
    int index = 0;
    while (elapsed.elapsed() < 420) {
      image.fill(QColor(++index % 255, 0, 0));
      QVERIFY(QMetaObject::invokeMethod(&widget, "onImageReceived", Q_ARG(QImage, image)));
      widget.repaint(); // Parent animations must not bypass the cap.
      QTest::qWait(5);
    }
    QVERIFY2(frames.count() <= 5, qPrintable(QString("Presented %1 frames in 420 ms at a 10 FPS cap").arg(frames.count())));
    QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() && qvariant_cast<QImage>(frames.last().at(0)) == image, 400);
    widget.stopVideo();
  }

  void testShouldRotateVideo180ForRobotId_data() {
    QTest::addColumn<int>("robotId");
    QTest::addColumn<bool>("expected");

    QTest::newRow("red-aerial") << 6 << true;
    QTest::newRow("blue-aerial") << 106 << true;
    QTest::newRow("red-hero") << 1 << false;
    QTest::newRow("blue-infantry") << 103 << false;
  }

  void testShouldRotateVideo180ForRobotId() {
    QFETCH(int, robotId);
    QFETCH(bool, expected);

    QCOMPARE(RM::VideoBackgroundWidget::shouldRotateVideo180ForRobotId(robotId),
             expected);
  }

  void testSetCurrentRobotIdUpdatesRotationState() {
    RM::VideoBackgroundWidget widget;

    widget.setCurrentRobotId(6);
    QVERIFY(widget.rotatesVideo180());

    widget.setCurrentRobotId(106);
    QVERIFY(widget.rotatesVideo180());

    widget.setCurrentRobotId(3);
    QVERIFY(!widget.rotatesVideo180());
  }
};

QTEST_MAIN(TestVideoBackgroundWidget)
#include "test_video_background_widget.moc"
