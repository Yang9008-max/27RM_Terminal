// SPDX-License-Identifier: MIT
// RMUL 2027 settlement; reuses the Fudan Qt window and viewport policy.
#include "GameResultWidget.h"
#include "../ui/PopupOverlayPolicy.h"
#include <QPainter>
#include <QKeyEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>
#include <QCollator>
#include <QPainterPath>
#include <algorithm>

namespace {
QString number(const std::optional<quint32> &value) {
  return value ? QString::number(*value) : QStringLiteral("—");
}
QString total(const std::array<std::optional<quint32>, 3> &hp) {
  quint64 sum = 0;
  for (const auto &value : hp) {
    if (!value) return QStringLiteral("—");
    sum += *value;
  }
  return QString::number(sum);
}
QString reason(quint8 value) {
  switch (value) {
    case 1: return QStringLiteral("裁判下发原因：基地被摧毁");
    case 2: return QStringLiteral("判定依据：控制度");
    case 3: return QStringLiteral("判定依据：全队总攻击伤害");
    case 4: return QStringLiteral("判定依据：全队机器人总剩余血量");
    case 5: return QStringLiteral("判定依据：平局");
    case 6: return QStringLiteral("异常终止");
    case 7: return QStringLiteral("主裁判手动判定胜负");
    default: return QStringLiteral("等待结束原因");
  }
}
}

GameResultWidget::GameResultWidget(QWidget *parent) : QWidget(parent), m_sound(new QSoundEffect(this)) {
  setObjectName("rmulSettlementPanel");
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_TranslucentBackground);
  addLabel("heading", {40, 24, 1120, 36}, 20, "#aabed4");
  addLabel("result", {230, 70, 740, 70}, 48);
  addLabel("reason", {40, 146, 1120, 38}, 22);
  addLabel("series", {40, 193, 1120, 38}, 20);
  addLabel("redTitle", {60, 243, 340, 36}, 26, "#ff6879");
  addLabel("blueTitle", {800, 243, 340, 36}, 26, "#58b9ff");
  const QStringList metrics{"controlPoints", "damage", "healthTotal"};
  const QStringList captions{QStringLiteral("控制度"), QStringLiteral("全队总攻击伤害"), QStringLiteral("1 / 3 / 7 总剩余血量")};
  for (int i = 0; i < metrics.size(); ++i) {
    const int y = 295 + i * 82;
    addLabel("red_" + metrics[i], {60, y, 340, 52}, i == 0 ? 38 : 28, "#ff6879");
    addLabel("blue_" + metrics[i], {800, y, 340, 52}, i == 0 ? 38 : 28, "#58b9ff");
    addLabel(metrics[i] + "Caption", {410, y, 380, 52}, 21);
    m_labels[metrics[i] + "Caption"]->setText(captions[i]);
  }
  const QStringList roles{QStringLiteral("1号 重装"), QStringLiteral("3号 步兵"), QStringLiteral("7号 哨兵")};
  for (int i = 0; i < 3; ++i) {
    const int y = 555 + i * 53;
    addLabel(QStringLiteral("red_hp%1").arg(i), {150, y, 250, 40}, 25, "#ff6879");
    addLabel(QStringLiteral("blue_hp%1").arg(i), {800, y, 250, 40}, 25, "#58b9ff");
    addLabel(QStringLiteral("role%1").arg(i), {410, y, 380, 40}, 21);
    m_labels[QStringLiteral("role%1").arg(i)]->setText(roles[i]);
  }
  addLabel("footer", {40, 725, 1120, 40}, 15, "#91a4bb");
  m_labels["footer"]->setText(QStringLiteral("裁判系统比赛战报     /     Esc 关闭     ·     Ctrl+R 查看结算"));
  m_labels["redTitle"]->setText(QStringLiteral("红方"));
  m_labels["blueTitle"]->setText(QStringLiteral("蓝方"));
  const QString sound = ":/sounds/game_finish.mp3";
  if (QFileInfo::exists(sound)) m_sound->setSource(QUrl("qrc" + sound));
  m_sound->setVolume(0.5);
  m_victoryOverlay = new QLabel(this);
  m_victoryOverlay->setObjectName("victoryAnimation");
  m_victoryOverlay->setAlignment(Qt::AlignCenter);
  m_victoryOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_victoryOverlay->hide();
  m_frameTimer.setInterval(16);
  connect(&m_frameTimer, &QTimer::timeout, this, &GameResultWidget::advanceVictoryFrame);
  m_barAnimation.setDuration(850);
  m_barAnimation.setStartValue(0.0);
  m_barAnimation.setEndValue(1.0);
  m_barAnimation.setEasingCurve(QEasingCurve::OutCubic);
  connect(&m_barAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
    m_reveal = value.toReal();
    update();
  });
  for (const QString &team : {QStringLiteral("red"), QStringLiteral("blue")}) {
    for (const QString &type : {QStringLiteral("hero"), QStringLiteral("infantry"), QStringLiteral("sentry")}) {
      const QString name = team + "_" + type;
      QPixmap icon(":/images/robots/" + name + ".png");
      if (icon.isNull()) icon.load(QCoreApplication::applicationDirPath() + "/resources/images/robots/" + name + ".png");
      m_robotIcons[name] = icon;
    }
  }
  resize(designSize());
  setSnapshot({});
}

void GameResultWidget::addLabel(const QString &name, const QRect &rect, int size, const QString &color) {
  auto *label = new QLabel(this);
  label->setObjectName(name);
  label->setAlignment(Qt::AlignCenter);
  label->setStyleSheet("color:" + color + ";background:transparent;");
  label->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_labels[name] = label;
  m_rects[name] = rect;
  m_fontSizes[name] = size;
}

void GameResultWidget::setSnapshot(const RmulSettlementSnapshot &snapshot) {
  m_snapshot = snapshot;
  if (!snapshot.inSettlement || (snapshot.winner > 2 && snapshot.endReason != 6)) {
    m_frameTimer.stop();
    m_barAnimation.stop();
    m_victoryOverlay->hide();
    m_framePaths.clear();
    m_announcedKey.clear();
    m_reveal = 1;
    for (auto *label : m_labels) label->show();
  }
  m_labels["heading"]->setText(QStringLiteral("RMUL 2027 · 3V3 · 第 %1 局结算").arg(snapshot.round));
  QString title = QStringLiteral("等待裁判结果");
  QString color = "#e7eef8";
  if (snapshot.inSettlement && snapshot.endReason == 6) title = QStringLiteral("比赛异常终止");
  else if (snapshot.inSettlement && snapshot.winner == 1) { title = QStringLiteral("红方获胜"); color = "#ff6879"; }
  else if (snapshot.inSettlement && snapshot.winner == 2) { title = QStringLiteral("蓝方获胜"); color = "#58b9ff"; }
  else if (snapshot.inSettlement && snapshot.winner == 0) title = QStringLiteral("本局平局");
  m_labels["result"]->setText(title);
  m_labels["result"]->setStyleSheet("color:" + color + ";background:transparent;");
  m_labels["reason"]->setText(snapshot.inSettlement ? reason(snapshot.endReason) : QStringLiteral("尚未进入结算阶段"));
  QString elapsed = QStringLiteral("—");
  if (snapshot.elapsed) elapsed = QStringLiteral("%1:%2").arg(*snapshot.elapsed / 60, 2, 10, QChar('0')).arg(*snapshot.elapsed % 60, 2, 10, QChar('0'));
  m_labels["series"]->setText(QStringLiteral("累计比分  红 %1 : %2 蓝     本局用时 %3").arg(snapshot.redScore).arg(snapshot.blueScore).arg(elapsed));
  m_labels["red_controlPoints"]->setText(number(snapshot.redControlPoints));
  m_labels["blue_controlPoints"]->setText(number(snapshot.blueControlPoints));
  m_labels["red_damage"]->setText(number(snapshot.redDamage));
  m_labels["blue_damage"]->setText(number(snapshot.blueDamage));
  m_labels["red_healthTotal"]->setText(total(snapshot.redHealth));
  m_labels["blue_healthTotal"]->setText(total(snapshot.blueHealth));
  for (int i = 0; i < 3; ++i) {
    m_labels[QStringLiteral("red_hp%1").arg(i)]->setText("HP " + number(snapshot.redHealth[i]));
    m_labels[QStringLiteral("blue_hp%1").arg(i)]->setText("HP " + number(snapshot.blueHealth[i]));
  }
  setAccessibleName(title);
  announceResult();
  update();
}

void GameResultWidget::setResolutionViewport(const QSize &viewport) {
  m_viewport = viewport.isValid() && !viewport.isEmpty() ? viewport : QSize(1920, 1080);
  resize(RM::PopupOverlayPolicy::scaledSize(designSize(), m_viewport));
}
void GameResultWidget::setSoundVolume(qreal volume) { m_sound->setVolume(qBound(0.0, volume, 1.0)); }
void GameResultWidget::announceResult() {
  if (!isVisible() || !m_snapshot.inSettlement || (m_snapshot.winner > 2 && m_snapshot.endReason != 6)) return;
  const QString key = QStringLiteral("%1/%2/%3").arg(m_snapshot.round).arg(m_snapshot.winner).arg(m_snapshot.endReason == 6);
  if (m_announcedKey == key) return;
  m_announcedKey = key;
  if (m_playSound && !qEnvironmentVariableIsSet("RM_DISABLE_AUDIO") && m_sound->source().isValid()) m_sound->play();
  emit resultAnnounced();
  startVictoryAnimation();
}
void GameResultWidget::showEvent(QShowEvent *event) { QWidget::showEvent(event); announceResult(); }
void GameResultWidget::hideEvent(QHideEvent *event) {
  QWidget::hideEvent(event);
  m_frameTimer.stop();
  m_barAnimation.stop();
  m_victoryOverlay->hide();
  m_framePaths.clear();
  for (auto *label : m_labels) label->show();
  m_announcedKey.clear();
  m_sound->stop();
}
void GameResultWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) { hide(); emit closeRequested(); event->accept(); return; }
  QWidget::keyPressEvent(event);
}
void GameResultWidget::resizeEvent(QResizeEvent *event) { QWidget::resizeEvent(event); layoutLabels(); }
void GameResultWidget::layoutLabels() {
  const qreal sx = width() / 1200.0, sy = height() / 800.0;
  for (auto it = m_labels.begin(); it != m_labels.end(); ++it) {
    const auto r = m_rects[it.key()];
    it.value()->setGeometry(qRound(r.x()*sx), qRound(r.y()*sy), qRound(r.width()*sx), qRound(r.height()*sy));
    QFont f("Microsoft YaHei");
    f.setPixelSize(qMax(1, qRound(m_fontSizes[it.key()] * qMin(sx, sy))));
    f.setBold(m_fontSizes[it.key()] >= 25);
    it.value()->setFont(f);
  }
  m_victoryOverlay->setGeometry(rect());
  if (m_frameTimer.isActive()) advanceVictoryFrame();
}
void GameResultWidget::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.scale(width() / 1200.0, height() / 800.0);
  const QColor red("#ff586b"), blue("#39acff"), gold("#f4cf80");
  QPainterPath panel;
  panel.moveTo(20, 1); panel.lineTo(1180, 1); panel.lineTo(1199, 20);
  panel.lineTo(1199, 780); panel.lineTo(1180, 799); panel.lineTo(20, 799);
  panel.lineTo(1, 780); panel.lineTo(1, 20); panel.closeSubpath();
  QLinearGradient background(0, 0, 0, 800);
  background.setColorAt(0, QColor("#1c2635"));
  background.setColorAt(0.45, QColor("#0f1723"));
  background.setColorAt(1, QColor("#080d14"));
  painter.fillPath(panel, background);
  if (m_frameTimer.isActive()) return;
  painter.setClipPath(panel);
  QLinearGradient glow(0, 0, 1200, 0);
  glow.setColorAt(0, QColor(255, 60, 85, 48));
  glow.setColorAt(0.48, Qt::transparent); glow.setColorAt(0.52, Qt::transparent);
  glow.setColorAt(1, QColor(30, 148, 255, 48));
  painter.fillRect(QRectF(0, 0, 1200, 235), glow);
  painter.setPen(QColor(130, 166, 199, 9));
  for (int x = -800; x < 1200; x += 32) painter.drawLine(x, 800, x + 800, 0);
  painter.setPen(QPen(QColor("#445568"), 1.5)); painter.drawPath(panel);
  painter.setPen(QPen(red, 3)); painter.drawLine(28, 1, 420, 1);
  painter.setPen(QPen(blue, 3)); painter.drawLine(780, 1, 1172, 1);
  painter.setPen(QColor("#586879"));
  painter.drawLine(44, 232, 1156, 232);
  if (m_snapshot.inSettlement && m_snapshot.endReason != 6 && (m_snapshot.winner == 1 || m_snapshot.winner == 2)) {
    const bool redWon = m_snapshot.winner == 1;
    QRectF badge(redWon ? 48 : 997, 91, 155, 46);
    painter.fillRect(badge, QColor(244, 207, 128, 14));
    painter.setPen(QPen(gold, 1)); painter.drawRect(badge);
    QFont font("Bahnschrift", 18, QFont::Bold); font.setPixelSize(21); font.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    painter.setFont(font); painter.drawText(badge, Qt::AlignCenter, "WINNER");
  }
  const std::array<std::optional<quint32>, 3> rv{m_snapshot.redControlPoints, m_snapshot.redDamage, std::nullopt};
  const std::array<std::optional<quint32>, 3> bv{m_snapshot.blueControlPoints, m_snapshot.blueDamage, std::nullopt};
  for (int i = 0; i < 3; ++i) {
    const qreal y = 290 + i * 82;
    const bool decisive = m_snapshot.inSettlement && m_snapshot.endReason == i + 2;
    painter.fillRect(QRectF(48, y, 1104, 73), decisive ? QColor(244, 207, 128, 13) : QColor(130, 160, 200, 7));
    if (decisive) {
      painter.setPen(QColor(244, 207, 128, 60)); painter.drawRect(QRectF(48, y, 1104, 73));
    }
    auto redValue = i == 2 ? total(m_snapshot.redHealth) : number(rv[i]);
    auto blueValue = i == 2 ? total(m_snapshot.blueHealth) : number(bv[i]);
    const bool known = redValue != QStringLiteral("—") && blueValue != QStringLiteral("—");
    const double maxValue = qMax(1.0, qMax(redValue.toDouble(), blueValue.toDouble()));
    for (bool isRed : {true, false}) {
      const qreal x = isRed ? 72 : 788;
      painter.fillRect(QRectF(x, y + 60, 340, 4), QColor("#28313d"));
      const qreal fraction = known ? (isRed ? redValue.toDouble() : blueValue.toDouble()) / maxValue : 0;
      const qreal length = 340 * fraction * m_reveal;
      QLinearGradient bar(x, 0, x + 340, 0);
      bar.setColorAt(0, isRed ? QColor("#823045") : QColor("#2786df"));
      bar.setColorAt(1, isRed ? red : QColor("#1c4169"));
      painter.fillRect(QRectF(isRed ? x + 340 - length : x, y + 60, length, 4), bar);
    }
  }
  const QStringList types{"hero", "infantry", "sentry"};
  for (int i = 0; i < 3; ++i) {
    const qreal y = 550 + i * 53;
    painter.fillRect(QRectF(48, y, 1104, 48), QColor(130, 160, 200, 10));
    painter.setPen(QColor("#29394b")); painter.drawLine(420, y + 8, 420, y + 40); painter.drawLine(780, y + 8, 780, y + 40);
    for (bool isRed : {true, false}) {
      const auto icon = m_robotIcons.value((isRed ? "red_" : "blue_") + types[i]);
      if (!icon.isNull()) painter.drawPixmap(QRect(isRed ? 85 : 1070, y + 2, 44, 44), icon);
    }
  }
}

void GameResultWidget::startVictoryAnimation() {
  m_frameTimer.stop();
  m_framePaths.clear();
  m_frameIndex = -1;
  QString directory;
  if (m_snapshot.endReason == 6) directory = "abnormal_termination_zh";
  else if (m_snapshot.winner == 1) directory = "red_win_zh";
  else if (m_snapshot.winner == 2) directory = "blue_win_zh";
  if (!directory.isEmpty() && !qEnvironmentVariableIsSet("RM_DISABLE_RESULT_ANIMATION")) {
    const QString app = QCoreApplication::applicationDirPath();
    const QStringList roots{":/images/resultpanel/", app + "/resources/images/resultpanel/",
      app + "/../resources/images/resultpanel/", app + "/../../resources/images/resultpanel/",
      app + "/../../../resources/images/resultpanel/", QDir::currentPath() + "/resources/images/resultpanel/"};
    for (const auto &root : roots) {
      QDir dir(root + directory);
      auto files = dir.entryInfoList({"*.png"}, QDir::Files);
      if (files.isEmpty()) continue;
      QCollator collator; collator.setNumericMode(true);
      std::sort(files.begin(), files.end(), [&](const auto &a, const auto &b) { return collator.compare(a.fileName(), b.fileName()) < 0; });
      for (const auto &file : files) m_framePaths.append(file.absoluteFilePath());
      break;
    }
  }
  if (m_framePaths.isEmpty()) { finishVictoryAnimation(); return; }
  for (auto *label : m_labels) label->hide();
  m_victoryOverlay->setGeometry(rect());
  m_victoryOverlay->show();
  m_victoryOverlay->raise();
  m_frameClock.start();
  m_frameTimer.start();
  advanceVictoryFrame();
  emit victoryAnimationStarted(m_snapshot.winner);
}

void GameResultWidget::advanceVictoryFrame() {
  const int index = static_cast<int>(m_frameClock.elapsed() / 16);
  if (index >= m_framePaths.size()) { finishVictoryAnimation(); return; }
  if (index == m_frameIndex) return;
  m_frameIndex = index;
  const QPixmap frame(m_framePaths[index]);
  if (!frame.isNull()) m_victoryOverlay->setPixmap(frame.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void GameResultWidget::finishVictoryAnimation() {
  m_frameTimer.stop();
  m_victoryOverlay->hide();
  for (auto *label : m_labels) label->show();
  m_barAnimation.stop();
  if (qEnvironmentVariableIsSet("RM_DISABLE_RESULT_ANIMATION")) m_reveal = 1;
  else m_barAnimation.start();
  update();
}
