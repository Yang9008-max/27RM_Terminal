# Local Settings Implementation Plan

> **For agentic workers:** Execute the following tasks inline in this session. The user has authorized implementation; unavailable execution sub-skills and delegation are not required.

**Goal:** Make P-panel audio and video controls change real outputs and explain the scope of mouse sensitivity.

**Architecture:** Keep existing MainWindow routing. Synchronize runtime values and audio availability to QML, route prompt volume to every existing output, and provide independent local audio previews. Video presentation retains the newest pending frame and enforces its selected cap even during unrelated repaints.

**Tech Stack:** Qt Widgets / Quick / Multimedia, C++17, QtTest, Windows UCRT64.

## Global Constraints

- Repository: D:\MyTrain\projects\27RM_Terminal; RMUL 2027 3V3.
- Baseline: rmul2027-pre-05-local-settings-20261114; branch: rmul2027/05-local-settings.
- Do not bundle missing historical music or fabricate match voice assets. A generated neutral preview tone is explicitly a settings test sound.
- Normal delivery must run with audio enabled; RM_DISABLE_AUDIO remains available for automated tests.
- Settings are local runtime controls, not referee protocol messages or Windows-wide pointer/audio settings.

## Task 1: Reproduce output failures

- [x] Extend main-window integration tests: prompt volume 23 must reach the secondary voice output, background volume must start at 50%, and RM_DISABLE_AUDIO=0 must permit audio.
- [x] Register the existing video widget tests with CTest. Feed frames faster than a 10 FPS cap, including forced repaints; assert presented frames remain capped and the final pending frame eventually appears.
- [x] Run the tests before fixes and record the failures.

## Task 2: Connect controls to real behavior

- [x] MainWindow.cpp/.h: clamp and retain both volume values; apply prompt volume to voice/kill/result/preview outputs; preserve the selected value across warmup.
- [x] Add a neutral generated WAV preview resource and independent preview player. Add local music selection and preview without interrupting referee voice queues. Reject absent files, show decoder errors, and use selected music during match stages.
- [x] SettingsPanel.qml: show synchronized values, device/missing-source status, test buttons, mouse gain and video-only FPS wording. Guard slider value changes against the synchronized model so accessibility edits work without initialization feedback; use onActivated for FPS selection.
- [x] VideoBackgroundWidget.cpp/.h: schedule by elapsed monotonic time, keep newest pending image, and guard presentation inside paintEvent so parent repaints cannot bypass the cap. Changing the cap reschedules pending presentation.

## Task 3: Verify and deliver

- [x] Run Release build and full CTest; test zero/clamped audio values and missing-source/disabled-audio UI states.
- [x] Launch the actual Windows client with RM_DISABLE_AUDIO unset; operate sliders and preview controls through agent-desktop. Record screenshots and limits of audible verification.
- [x] Update development documentation and AGENTS.md, then commit and tag rmul2027-step-05-done.
