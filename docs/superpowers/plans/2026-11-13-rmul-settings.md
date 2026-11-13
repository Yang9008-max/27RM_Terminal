# 2027 RMUL P键设置迁移 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 完整检查P设置面板，去除2026英雄配置和不适用于RMUL 3V3的控制入口，使性能设置、身份选择与2027协议一致。

**Architecture:** 保留Qt/QML面板布局及旧英文身份键以兼容现有图传/兑换路径，对外显示中文RMUL角色。QML按当前登录身份显示性能入口，C++与网络出口再次限制有效枚举。服务端回执单独更新当前机器人性能，不由本地点击冒充确认。

**Tech Stack:** Qt Widgets、QML、QtTest、Protobuf optional、Python/Paho、agent-desktop Windows。

## Global Constraints

- 基线7161cf8，标签rmul2027-pre-04-settings-20261113，分支rmul2027/04-settings；用户授权直接执行，本次不另派代理。
- 规则V1.0.0 PDF19：仅步兵可选血量/功率、冷却/爆发；战斗开始后固定。PDF17–18/38：重装性能随装配激活；PDF21：哨兵全自动。
- 协议V1.0.0 PDF61/70–71：性能枚举只有1/2，Command及Sync字段为optional。0在UI中表示不选择，发送时省略，不能发旧3/4枚举或哨兵半自动。
- 保留所有通用本地显示、声音、灵敏度、帧率和图传功能；去掉伪造的温度45°C/通道0。未验证的新机器人功能不以改名伪装完成。
- 只有红/蓝1、3、7可在RMUL设置中选择；哨兵入口用于观测，不允许键鼠控制。
- 停止自己的客户端及模拟器后构建/CTest；测试完成再启动供用户查看。重大修改前已有Git版本，无需重复询问。

### Task 1: 回归与角色/指令约束

**Files:** src/core/RmulSettingsPolicy.h、tests/unit/test_settings_panel_shortcuts.cpp、tests/unit/test_input_hotkey_policy.cpp、tests/CMakeLists.txt。

- [x] 将原十身份快捷键用例改为六身份：数字1–6对应红1/3/7、蓝1/3/7；0/7/8/9不选择。
- [x] 添加真实QML回归：重装/哨兵无性能控制，步兵只接受1/2；未登录、身份未匹配、战斗阶段不发命令，初始化/协议回执/换身份不发命令。
- [x] 先运行并记录旧代码失败，再实现统一C++规则约束：`isRosterRobot(id)`、`canSelectPerformance(id, loggedIn, stage)`、`validPerformanceSelection(shooter, chassis, sentryControl)`。

### Task 2: 迁移设置读写链路

**Files:** src/qml/SettingsPanel.qml、src/ui/MainWindow.cpp、src/ui/InputHotkeyPolicy.cpp、src/network/proto/robomaster.proto、src/network/NetworkManager.cpp、src/core/GameData.h/.cpp。

- [x] 选择器仅保留六个RMUL角色，内部英文键兼容旧路径，显示中文和快捷键说明；重装显示装配激活说明，哨兵显示全自动及观测说明。
- [x] 性能模型固定为初始/1/2，删除英雄近战/远程；只在用户onActivated时请求，绑定回执和初始化不触发发送。
- [x] 移除哨兵半自动选项和假温度/模式/通道，保留本地设置；修改P面板中会误导为17mm重装性能的说明。
- [x] Command/Sync三个字段改为官方optional；发送只写非零性能字段，不写哨兵控制字段；在C++槽及网络出口拒绝非法枚举。
- [x] 接入性能Sync并按presence更新；StaticStatus中缺失性能字段保留旧值，未知枚举不溢出成有效值；只展示收到的确认值，身份切换不带入上台草稿。
- [x] 检查登录、键鼠和性能调用链，哨兵只观测，旧工程/空中身份不从P入口进入；不扩展成全客户端重写。

### Task 3: 验证与交付

**Files:** docs/development/rmul2027-settings.md、AGENTS.md、现有测试与本地忽略目录验收脚本。

- [x] 运行受影响QtTest及完整Release/CTest；用本地MQTT确认有效性能Command字段、无效请求不发布、Sync不回发命令。
- [x] 用agent-desktop实际按P，逐一查看角色与性能下拉框，确认红蓝身份、步兵可选、重装/哨兵提示、布局与关闭均正常；保存截图。
- [x] 记录所有P面板项目的处理结论、依据、测试和剩余旧页面边界；`git diff --check`，提交并创建rmul2027-step-04-done标签。
