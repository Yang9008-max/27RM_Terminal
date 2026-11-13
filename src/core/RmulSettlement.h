// SPDX-License-Identifier: MIT
#pragma once

#include <QtGlobal>
#include <array>
#include <optional>

// A received snapshot, never a client-side adjudication. Missing is distinct
// from zero for every statistic. Roster order is heavy 1, infantry 3, sentry 7.
struct RmulSettlementSnapshot {
  bool inSettlement = false;
  quint8 winner = 255;
  quint8 endReason = 255;
  int round = 1;
  int redScore = 0;
  int blueScore = 0;
  std::optional<quint32> elapsed;
  std::optional<quint32> redControlPoints, blueControlPoints;
  std::optional<quint32> redDamage, blueDamage;
  std::array<std::optional<quint32>, 3> redHealth{}, blueHealth{};
};
