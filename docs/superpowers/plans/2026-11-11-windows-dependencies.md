# Windows Dependencies Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Install and verify the dependencies needed to build and run the Fudan client and its simulator on Windows.

**Architecture:** Keep a portable MSYS2 UCRT64 environment in `.deps/msys64` and a Python environment in `sim/.venv`. Use local CMake configuration and launch scripts so the existing system toolchain is unaffected.

**Tech Stack:** GCC, CMake, Ninja, Qt 6, Protobuf, Abseil, Paho MQTT, FFmpeg, Mosquitto, Python 3.13, Node.js.

## Global Constraints

- Repository: `D:\MyTrain\projects\27RM_Terminal`.
- Preserve upstream protocol behavior; 2027 RMUL adaptation is outside dependency setup.
- Use only a local MQTT broker for validation.
- Keep downloaded SDKs, build output, and virtual environments untracked.

### Task 1: Install an isolated native SDK

**Files:** `.deps/msys64/`, `.git/info/exclude`.

- [x] Download the official MSYS2 base archive and verify SHA-256 against the official GitHub release (the existing package keyring does not contain the installer signing key).
- [x] Extract under `.deps`, initialize the environment, and perform a complete package update using `pacman -Syu --noconfirm`.
- [x] Install UCRT64 GCC, CMake, Ninja, pkgconf, Qt base/declarative/multimedia/serialport/svg/shadertools/httpserver, Protobuf, Abseil, Paho MQTT C, FFmpeg, Mosquitto, and ccache.
- [x] Verify package versions and compiler/library discovery.

### Task 2: Install simulator dependencies

**Files:** `sim/.venv/`.

- [x] Run `py -3.13 -m venv sim/.venv`.
- [x] Run `sim/.venv/Scripts/python.exe -m pip install -e ./sim`.
- [x] Run `sim/.venv/Scripts/python.exe -m pip check` and import the declared modules.

### Task 3: Build, validate, and document launch commands

**Files:** `CMakeUserPresets.json`, `tools/windows/`, `docs/development/windows-setup.md`.

- [x] Configure a Ninja Release build with the isolated UCRT64 prefix and `BUILD_TESTING=ON`.
- [x] Build the client and tests; no upstream C++ changes were required.
- [x] Verify CTest (27/27), simulator core unit tests (22/22), and Node geometry tests (3/3); record the five POSIX shell tests that cannot run directly under native Windows Python.
- [x] Provide repeatable build/run commands and verify the GUI starts with local configuration; deploy the official ONNX DLL beside executable files to avoid the Windows System32 namesake.
- [x] Record optional WebEngine and model weight limitations separately from required dependencies.
