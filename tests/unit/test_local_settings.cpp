#include "ui/MainWindow.h"
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QQuickItem>
#include <QQuickWidget>
#include <QSoundEffect>
#include <QTemporaryDir>
#include <QFile>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QtTest>

class TestLocalSettings : public QObject {
  Q_OBJECT
private slots:
  void initTestCase() {
    Q_INIT_RESOURCE(qml);
    Q_INIT_RESOURCE(resources);
    qputenv("RM_DEVTOOLS", "0");
  }
  void testPromptVolumeReachesVoiceOutput() {
    qputenv("RM_DISABLE_AUDIO", "0");
    RM::MainWindow window;
    // Keep this synchronous: the zero-timer network startup must not run.
    QVERIFY(QMetaObject::invokeMethod(&window, "onSystemVolumeChanged", Q_ARG(int, 23)));
    const auto players = window.findChildren<QMediaPlayer *>();
    QVERIFY(players.size() >= 2);
    // Every non-BGM player is a prompt or settings preview player.
    for (int i = 1; i < players.size(); ++i) {
      QVERIFY(players[i]->audioOutput());
      QCOMPARE(players[i]->audioOutput()->volume(), 0.23f);
    }
    for (auto *effect : window.findChildren<QSoundEffect *>())
      QCOMPARE(effect->volume(), 0.23f);
    QVERIFY(QMetaObject::invokeMethod(&window, "onSystemVolumeChanged", Q_ARG(int, 0)));
    for (int i = 1; i < players.size(); ++i)
      QCOMPARE(players[i]->audioOutput()->volume(), 0.0f);
    QVERIFY(QMetaObject::invokeMethod(&window, "onSystemVolumeChanged", Q_ARG(int, 150)));
    for (int i = 1; i < players.size(); ++i)
      QCOMPARE(players[i]->audioOutput()->volume(), 1.0f);
  }
  void testBackgroundVolumeMatchesUiAndClamps() {
    qputenv("RM_DISABLE_AUDIO", "0");
    RM::MainWindow window;
    const auto players = window.findChildren<QMediaPlayer *>();
    QVERIFY(!players.isEmpty());
    auto *output = players.front()->audioOutput();
    QVERIFY(output);
    QCOMPARE(output->volume(), 0.5f);
    QVERIFY(QMetaObject::invokeMethod(&window, "onMusicVolumeChanged", Q_ARG(int, 0)));
    QCOMPARE(output->volume(), 0.0f);
    QVERIFY(QMetaObject::invokeMethod(&window, "onMusicVolumeChanged", Q_ARG(int, 75)));
    QCOMPARE(output->volume(), 0.75f);
  }
  void testDisabledAudioIsVisibleAndInactive() {
    qputenv("RM_DISABLE_AUDIO", "1");
    RM::MainWindow window;
    QCOMPARE(window.findChildren<QMediaPlayer *>().size(), 0);
    auto *panel = window.findChild<QQuickWidget *>(QStringLiteral("settingsPanelWidget"));
    // Find by source because the pre-existing container has no required name.
    if (!panel) for (auto *candidate : window.findChildren<QQuickWidget *>())
      if (candidate->source().fileName() == "SettingsPanel.qml") panel = candidate;
    QVERIFY(panel && panel->rootObject());
    auto *root = panel->rootObject();
    QVERIFY(!root->property("audioAvailable").toBool());
    QVERIFY(!root->property("audioStatus").toString().isEmpty());
    auto *slider = root->findChild<QQuickItem *>("promptVolumeSlider");
    auto *button = root->findChild<QQuickItem *>("promptPreviewButton");
    QVERIFY(slider && button);
    QVERIFY(!slider->isEnabled());
    QVERIFY(!button->isEnabled());
    for (auto *effect : window.findChildren<QSoundEffect *>())
      QCOMPARE(effect->volume(), 0.0f);
  }
  void testAccessibleSliderChangesReachOutputWithoutFeedback() {
    qputenv("RM_DISABLE_AUDIO", "0");
    RM::MainWindow window;
    QQuickWidget *panel = nullptr;
    for (auto *candidate : window.findChildren<QQuickWidget *>())
      if (candidate->source().fileName() == "SettingsPanel.qml") panel = candidate;
    QVERIFY(panel && panel->rootObject());
    auto *root = panel->rootObject();
    auto *slider = root->findChild<QObject *>("promptVolumeSlider");
    auto *output = window.findChild<QAudioOutput *>("promptAudioOutput");
    QVERIFY(slider && output);
    QSignalSpy changed(root, SIGNAL(volumeChanged(int)));
    // Screen-reader/UIA value changes use the value property rather than moved().
    slider->setProperty("value", 17);
    QCOMPARE(output->volume(), 0.17f);
    QCOMPARE(root->property("promptVolume").toInt(), 17);
    QCOMPARE(changed.count(), 1);
    changed.clear();
    root->setProperty("promptVolume", 31); // Model -> UI synchronization is silent.
    QCOMPARE(slider->property("value").toInt(), 31);
    QCOMPARE(changed.count(), 0);
  }
  void testLocalMusicAndPreviewHaveIndependentOutputs() {
    qputenv("RM_DISABLE_AUDIO", "0");
    RM::MainWindow window;
    QTemporaryDir temp;
    const QString path = temp.filePath("music.wav");
    QVERIFY(QFile::copy(":/settings-audio/preview.wav", path));
    bool accepted = false;
    QVERIFY(QMetaObject::invokeMethod(&window, "setBackgroundMusicFile", Q_RETURN_ARG(bool, accepted), Q_ARG(QString, path)));
    QVERIFY(accepted);
    auto *bgm = window.findChildren<QMediaPlayer *>().front();
    QCOMPARE(bgm->source(), QUrl::fromLocalFile(path));
    QVERIFY(QMetaObject::invokeMethod(&window, "setBackgroundMusicFile", Q_RETURN_ARG(bool, accepted), Q_ARG(QString, temp.filePath("missing.mp3"))));
    QVERIFY(!accepted);
    QCOMPARE(bgm->source(), QUrl::fromLocalFile(path));
    if (QMediaDevices::defaultAudioOutput().isNull()) QSKIP("No output device for preview routing test");
    QVERIFY(QMetaObject::invokeMethod(&window, "previewBackgroundMusic"));
    auto *preview = window.findChild<QMediaPlayer *>("settingsAudioPreview");
    QVERIFY(preview);
    QCOMPARE(preview->source(), QUrl::fromLocalFile(path));
    QVERIFY(QMetaObject::invokeMethod(&window, "onMusicVolumeChanged", Q_ARG(int, 14)));
    QCOMPARE(preview->audioOutput()->volume(), 0.14f);
    QVERIFY(QMetaObject::invokeMethod(&window, "onSystemVolumeChanged", Q_ARG(int, 80)));
    QCOMPARE(preview->audioOutput()->volume(), 0.14f);
    QVERIFY(QMetaObject::invokeMethod(&window, "previewPromptSound"));
    QCOMPARE(preview->audioOutput()->volume(), 0.8f);
    QCOMPARE(preview->source(), QUrl("qrc:/settings-audio/preview.wav"));
    QCOMPARE(bgm->source(), QUrl::fromLocalFile(path));
  }
  void testPreviewToneCanActuallyDecodeAndPlay() {
    // Isolate playback from MainWindow's deferred network startup.
    if (QMediaDevices::defaultAudioOutput().isNull()) QSKIP("No audio output device");
    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);
    output.setVolume(0); // Automated test verifies decoding without audible output.
    player.setSource(QUrl("qrc:/settings-audio/preview.wav"));
    player.play();
    QTRY_VERIFY_WITH_TIMEOUT(player.position() > 0 || player.mediaStatus() == QMediaPlayer::EndOfMedia, 5000);
    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QVERIFY(player.duration() >= 750 && player.duration() <= 850);
  }
};
QTEST_MAIN(TestLocalSettings)
#include "test_local_settings.moc"
