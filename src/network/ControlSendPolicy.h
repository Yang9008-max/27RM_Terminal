#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QThread>
#include <functional>

namespace RM {
// One independent 75 Hz budget per official control topic. The final queued
// state is sent even when the producer stops immediately after a release.
class ControlSendPolicy : public QObject {
public:
  using Publisher = std::function<bool(const QByteArray &)>;
  explicit ControlSendPolicy(Publisher publisher, QObject *parent = nullptr)
      : QObject(parent), m_publish(std::move(publisher)) {
    m_clock.start();
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this] { flush(); });
  }
  bool submit(const QByteArray &payload) {
    m_pending = payload;
    m_hasPending = true;
    m_retries = 0;
    return flush(); // true means accepted for delivery, including queued data.
  }
  QByteArray pendingPayload() const { return m_hasPending ? m_pending : QByteArray{}; }
  void finish() {
    // Shutdown only: drain within the rate budget and bounded retry count.
    for (int i = 0; i < 4 && m_hasPending; ++i) {
      const qint64 remaining = kIntervalNs - (m_clock.nsecsElapsed() - m_lastAttemptNs);
      if (m_lastAttemptNs >= 0 && remaining > 0)
        QThread::usleep(static_cast<unsigned long>((remaining + 999) / 1000));
      flush();
    }
  }
  void cancel() {
    m_timer.stop();
    m_pending.clear();
    m_hasPending = false;
    m_lastAttemptNs = -1;
  }
private:
  bool flush() {
    if (!m_hasPending) return true;
    const qint64 now = m_clock.nsecsElapsed();
    const qint64 wait = m_lastAttemptNs < 0 ? 0 : kIntervalNs - (now - m_lastAttemptNs);
    if (wait > 0) {
      m_timer.start(int((wait + 999999) / 1000000));
      return true;
    }
    m_timer.stop();
    const QByteArray payload = m_pending;
    m_hasPending = false;
    m_lastAttemptNs = now;
    if (m_publish(payload)) return true;
    if (++m_retries <= 3) {
      m_pending = payload;
      m_hasPending = true;
      m_timer.start(14);
    }
    return false;
  }
  static constexpr qint64 kIntervalNs = 13333334;
  Publisher m_publish;
  QElapsedTimer m_clock;
  QTimer m_timer;
  QByteArray m_pending;
  qint64 m_lastAttemptNs = -1;
  bool m_hasPending = false;
  int m_retries = 0;
};
} // namespace RM
