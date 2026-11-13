#pragma once

namespace RM::RmulSettingsPolicy {
inline bool isRosterRobot(int id) {
  return id == 1 || id == 3 || id == 7 || id == 101 || id == 103 || id == 107;
}
inline bool isInfantry(int id) { return id == 3 || id == 103; }
inline bool canSelectPerformance(int id, bool loggedIn, int stage) {
  return loggedIn && isInfantry(id) && stage >= 1 && stage <= 3;
}
inline bool validPerformanceSelection(int shooter, int chassis, int sentryControl) {
  return shooter >= 0 && shooter <= 2 && chassis >= 0 && chassis <= 2 &&
         (shooter != 0 || chassis != 0) && sentryControl == 0;
}
} // namespace RM::RmulSettingsPolicy
