# Transport State Review Fixes Implementation Plan

> **For agentic workers:** Implement this plan task-by-task in the current session. The user has authorized implementation; no execution handoff is needed.

**Goal:** Prevent stale liveness and dynamic robot state from overriding newer messages or leaking into a new connection.

**Architecture:** Keep the existing optional-field merge model within a session. Refresh the cached alive state when official dynamic/global health arrives, retain explicit off-field priority, and invalidate dynamic values at the existing session reset boundary.

**Tech Stack:** C++17, Qt Test, Protobuf, MQTT, existing Windows UCRT64 Release build.

## Global Constraints

- Baseline tag: `rmul2027-pre-07-state-review-20261118`, pointing to `4dd6767`.
- Preserve absent fields within a session; explicit zero/false must still apply.
- Preserve `field_state == 1` as off-field, including when positive health arrives.
- Keep 1/3/4/7 health slot parsing and red/blue perspective unchanged.
- Use `build/windows-ucrt64-transport`; do not stop the running client or shared simulator.
- Commit the completed changes and verification record, as required by `AGENTS.md`.

## Task 1: Reproduce and fix cross-topic liveness

**Files:** Modify `tests/unit/test_rmul2027_transport.cpp` and `src/core/GameData.cpp`.

**Interfaces:** `updateRobotStaticStatus`, `updateRobotDynamicStatus`, `updateGlobalUnitStatus`; reuse `m_robotStaticSnapshots` to retain effective alive state by robot ID.

- [x] Add data-driven regressions for red/blue, zero/positive health, and dynamic/global sources. Start with the opposite cached alive state, deliver health, then deliver only `field_state=2`; the last packet must preserve the newer liveness.
- [x] Add a regression proving that `field_state=1` remains OFFLINE after positive health and alive-only updates, until `field_state=2` arrives.
- [x] Build and run the new tests before implementation, recording failures.
- [x] On explicit health, update the same effective alive cache used by static merges:

```cpp
m_robotStaticSnapshots[robotId].set_alive_state(health > 0 ? 1 : 2);
```

- [x] In the dynamic status path, apply OFFLINE when cached field state is 1; otherwise derive NORMAL/DESTROYED from the explicit health. Do not touch the cache for heat-only messages.
- [x] Run the targeted tests and preserve later explicit static alive updates as newer input.

## Task 2: Invalidate dynamic values at the session boundary

**Files:** Modify `tests/unit/test_rmul2027_transport.cpp`, `src/core/GameData.cpp`, `docs/development/rmul2027-transport.md`, and `AGENTS.md`.

**Interfaces:** `resetMatchStatus` is already called by MQTT start/loss and role changes; no new network API is required.

- [x] Seed HP, heat, ammunition, experience, buffer, firing rate, out-of-combat state/countdown, and remote permissions. Verify a sparse update preserves them within the session.
- [x] Cover direct reset and role switch away/back on both teams. After reset, HP and dynamic counters must be cleared, permissions false, and status OFFLINE; a heat-only packet must not restore old permissions or HP.
- [x] Verify new explicit permission true/false and health zero still update after reset. Observe `myRobotUpdated`/`robotDataUpdated` notifications.
- [x] Clear the dynamic values and related transient timestamps/heat flags for every robot in `resetMatchStatus`, preserving identity and configured/static limits. Clear the local heat override timestamp. Notify affected robot views.
- [x] Extend the private-MQTT test with pre-disconnect permissions and a post-reconnect heat-only message; verify no previous permissions/health survive.
- [x] Run the focused transport test, full CTest suite, and `tools/verify_rmul2027_transport.py --receiver build/windows-ucrt64-transport/tests/test_rmul2027_transport.exe` against its private broker.
- [x] Record actual test results and remaining real-hardware limitations, inspect `git diff --check`, and commit the fix.
