// SPDX-License-Identifier: MIT
// RMUL 2027 settlement; reuses the Fudan Qt window and viewport policy.
#include "GameResultWidget.h"
#include "../ui/PopupOverlayPolicy.h"
#include <QPainter>
#include <QKeyEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QFileInfo>

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
  addLabel("heading", {40, 24, 1120, 36}, 22, "#aabed4");
  addLabel("result", {40, 70, 1120, 70}, 48);
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
    addLabel(QStringLiteral("red_hp%1").arg(i), {60, y, 340, 40}, 25, "#ff6879");
    addLabel(QStringLiteral("blue_hp%1").arg(i), {800, y, 340, 40}, 25, "#58b9ff");
    addLabel(QStringLiteral("role%1").arg(i), {410, y, 380, 40}, 21);
    m_labels[QStringLiteral("role%1").arg(i)]->setText(roles[i]);
  }
  addLabel("footer", {40, 725, 1120, 40}, 15, "#91a4bb");
  m_labels["footer"]->setText(QStringLiteral("结果以裁判系统下发为准 · 统计为本会话最新接收快照 · Esc 关闭"));
  m_labels["redTitle"]->setText(QStringLiteral("红方"));
  m_labels["blueTitle"]->setText(QStringLiteral("蓝方"));
  const QString sound = ":/sounds/game_finish.mp3";
  if (QFileInfo::exists(sound)) m_sound->setSource(QUrl("qrc" + sound));
  m_sound->setVolume(0.5);
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
}
void GameResultWidget::showEvent(QShowEvent *event) { QWidget::showEvent(event); announceResult(); }
void GameResultWidget::hideEvent(QHideEvent *event) { QWidget::hideEvent(event); m_announcedKey.clear(); m_sound->stop(); }
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
}
void GameResultWidget::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(QColor("#101c2c"));
  painter.setPen(QPen(QColor("#4c6686"), 2));
  painter.drawRoundedRect(rect().adjusted(1,1,-1,-1), 14,14);
  painter.setPen(QColor("#2c435e"));
  painter.drawLine(width()*0.06, height()*0.66, width()*0.94, height()*0.66);
}
