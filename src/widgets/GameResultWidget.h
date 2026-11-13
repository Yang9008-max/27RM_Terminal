// SPDX-License-Identifier: MIT
#pragma once
#include "../core/RmulSettlement.h"
#include <QWidget>
#include <QMap>
#include <QLabel>
#include <QSoundEffect>

class GameResultWidget : public QWidget {
  Q_OBJECT
public:
  explicit GameResultWidget(QWidget *parent = nullptr);
  void setSnapshot(const RmulSettlementSnapshot &snapshot);
  void setResolutionViewport(const QSize &viewport);
  QSize resolutionViewport() const { return m_viewport; }
  static QSize designSize() { return QSize(1200, 800); }
  void setPlayVictoryOnShow(bool enabled) { m_playSound = enabled; }
  bool playVictoryOnShow() const { return m_playSound; }
  void setSoundVolume(qreal volume);
signals:
  void closeRequested();
  void resultAnnounced();
protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;
  void showEvent(QShowEvent *) override;
  void hideEvent(QHideEvent *) override;
  void keyPressEvent(QKeyEvent *) override;
private:
  void addLabel(const QString &name, const QRect &rect, int size, const QString &color = "#e7eef8");
  void layoutLabels();
  void announceResult();
  QMap<QString, QLabel *> m_labels;
  QMap<QString, QRect> m_rects;
  QMap<QString, int> m_fontSizes;
  RmulSettlementSnapshot m_snapshot;
  QSize m_viewport{1920, 1080};
  QSoundEffect *m_sound;
  bool m_playSound = true;
  QString m_announcedKey;
};
