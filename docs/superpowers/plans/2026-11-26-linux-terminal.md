# Native Linux Terminal Implementation Plan

**Goal:** Build, install and run the RMUL 2027 desktop terminal on Ubuntu 24.04.

**Architecture:** Build the shared Qt application against native Linux libraries. Require MQTT and video dependencies in the Linux preset. Stage a standalone directory with a public config template, prompt audio, and the original settlement animation.

**Tech stack:** CMake, Ninja, C++17, Qt 6, Paho MQTT C, Protobuf, FFmpeg, Ubuntu 24.04.

## Task 1: Native build

- [x] Record baseline tag `rmul2027-pre-08-linux-20261126` at `5ef0c85`.
- [x] Configure the baseline using Ubuntu 24.04.4 x86_64 in WSL2. CMake found Qt 6.4.2, Protobuf 3.21.12, Paho and FFmpeg 6.1.1.
- [x] Add the Linux preset and require both match I/O libraries. Configurations with FFmpeg disabled and with an empty Paho SDK directory stopped with the corresponding dependency error.
- [x] Resolve the optional ONNX SDK header and native library. The Windows SDK configured and compiled the detection object. The Linux delivery builds with AR disabled.
- [x] Preserve LF in shell scripts when exporting or checking out on Windows. The Linux simulator dependency test passed with scripts exported from the staged Git tree.
- [x] Add Ubuntu x86_64 and arm64 CI jobs and a virtual audio device for the settings tests.
- [x] Run all 34 CTest targets on Ubuntu x86_64. The audio preview playback test passed using a virtual PulseAudio device.
- [x] Run 22 MQTT transport cases on a private Mosquitto broker with zero skips. The simulator to client probes passed 14 HUD frames and 26 settlement frames across both team colors.

## Task 2: Installed program

- [x] Stage the Linux executable, example config, audio, settlement assets and license notices. The package contains no local configuration.
- [x] Check the archive checksum, extract to a fresh directory and launch using the X11 Qt plugin under Xvfb. Confirm control points 600/590, both teams' health data, and that the generated UDP HEVC test image is decoded and drawn in the operation window.
- [x] Document dependency, startup and validation commands in [Linux 部署指南](../../getting-started/linux.md).

ARM64 CI is configured. The validated package targets Ubuntu 24.04 x86_64. Physical network and audio hardware and the real referee system still require validation on the target computer. The repository link audit reports the same nine pre-existing absolute local links as the baseline.
