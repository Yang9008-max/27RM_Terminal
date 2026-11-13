# 2027 RMUL 判胜与结算链路 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 让模拟器按2027 RMUL判胜，客户端使用官方结果，结算页显示控制度与双方1/3/7数据，并复现修复600:590却蓝胜的问题。

**Architecture:** 判胜仅在模拟器服务端执行；客户端以GameStatus的game_result和end_reason为结果来源，不根据比分、控制度或血量自行改判。共享类型RmulSettlementSnapshot把显式未知与真实0传给结算面板，主窗口的所有结算入口汇总到同一刷新函数。保留Qt框架，移除结算中的旧基地、前哨、空中、能量机关和虚构单车伤害。

**Tech Stack:** Python unittest、Qt Widgets/QtTest、Protobuf optional、MQTT、agent-desktop Windows。

## 用户视觉修正（2026-11-13）

功能迁移基线已提交 `a6d7d51`，标签 `rmul2027-pre-03-settlement-visual-20261113`。用户明确要求保留原框架漂亮的UI和动画，纯文字验收表不是最终交付。已复用现有红/蓝胜利PNG序列作为入场，之后展开带机器人图标、红蓝对比条、胜方高亮的3V3战报。仅有真实结果时播放；同局统计更新不重播；待定与平局不误播胜利；Esc可关闭。已新增动画生命周期和主窗口结果自动展示/关闭回归，并完成重构建、完整CTest、MQTT和真实桌面验收。

## Global Constraints

- 基线639fd2c，标签rmul2027-pre-03-settlement-20261113，分支rmul2027/03-settlement；本地提交已授权。
- 规则V1.0.0（20261015）PDF38/39：控制度→攻击伤害→1/3/7总剩余血量→平局，控制度一方归零立即结束。
- 协议V1.0.0（20261029）PDF54：结果0/1/2，仅阶段5有效；原因2控制度、3伤害、4血量、5平局、6异常、7手动；255未知。
- 模拟器战斗300秒；保留结算时的战斗用时；重复结算不得重复加分。缺失控制度或非法双方同为0时不制造结果。
- 只连接本地测试服务，不连接真实机器人；协议字段不自行增补，缺失字段不得默认为平局或0。
- 当前会话直接执行并自审；两个superpowers执行子技能未安装，不新增代理任务。

### Task 1: 模拟器判胜和生命周期

**Files:** Modify sim/server/state_manager.py, sim/server/mqtt_publisher.py, sim/server/match_simulation_engine.py; Create sim/tests/test_rmul2027_settlement.py; update现有协议及演示测试。

- [x] 先写回归：600/590且蓝方血量更高时winner=1、end_reason=2；同控制度分别比较攻击伤害和仅1/3/7血量（4号不同不影响）；全相同平局。
- [x] 运行 `sim/.venv/Scripts/python.exe -m unittest discover -s sim/tests -p test_rmul2027_settlement.py`，记录原逻辑失败。
- [x] 增加集中裁决接口，返回(winner, reason)，缺失或非法输入返回未知；更新所有进入结算入口，统一只加分一次，未知时发送255并允许补齐数据后裁决。
- [x] 修正开局600、战斗300秒、归零立即结算、下一局清理、战斗用时保留；演示时钟统一300秒正式战斗，不再由建筑血量决定胜负。
- [x] 跑全部Python测试并记录规则演示仍未实现控制区自动扣分等边界；与客户端状态和首版面板合并提交为功能基线a6d7d51。

### Task 2: 客户端官方结果与数据投影

**Files:** Create src/core/RmulSettlement.h; Modify src/core/GameData.h/.cpp, tests/unit/test_game_data.cpp; add tests/unit/test_rmul2027_settlement.cpp and tests/CMakeLists.txt。

- [x] 修改旧的比分判胜测试为官方结果测试：蓝方总比分领先但收到红胜，必须红胜；无结果为255，明确0为平局。
- [x] 覆盖结果延迟、稀疏字段、重复快照、非结算255、未知枚举、直接连入结算、新局/断线/身份切换清理；旧GameResult事件不能抢先结束比赛。
- [x] 实现显式结果/原因/用时有效性；读取optional字段并保留同会话同局已收到值。只在GameStatus支持的结算阶段确认结果，不以其他topic猜胜者。
- [x] 通过RmulSettlementSnapshot投影官方控制度、uint32攻击伤害、固定1/3/7血量、累计比分和局数；未知数值为std::nullopt，协议4号不计入RMUL总血量。
- [x] 编译并运行GameData及新结算接收测试，先失败后通过；与模拟器合并提交为功能基线a6d7d51。

### Task 3: RMUL结算面板及主窗口整合

**Files:** Modify src/widgets/GameResultWidget.h/.cpp, src/ui/MainWindow.h/.cpp, tests/unit/test_game_result_widget.cpp。

- [x] 为结算面板新增setSnapshot(const RmulSettlementSnapshot&)；用真实标签暴露结果、原因、比分、控制度、伤害、1/3/7血量和用时，未知显示—或等待裁判结果。
- [x] 替换旧绘制中的基地/前哨/能量机关/六车统计；不推导单车伤害、击杀、MVP。保留尺寸适配、声音开关；普通统计更新不反复播放音效。
- [x] 主窗口合并阶段进入、结果变化、后续统计更新入口；清理旧重复连接及Ctrl+R虚构演示数据，快捷键只查看当前结算快照。
- [x] 测试600/590红胜、蓝胜/平局/异常/待定、0与未知、uint32极值、当前桌面及缩放尺寸适配；确认重复刷新不重开面板或重复发声。

### Task 4: 集成、真实桌面和全面复核

**Files:** Extend tools/verify_rmul2027_data.py with settlement receiver scenario or create tools/verify_rmul2027_settlement.py; Create docs/development/rmul2027-settlement.md; update AGENTS.md和HUD边界。

- [x] 用生产MQTT发布接口→NetworkManager→GameData→GameResultWidget验证红/蓝视角、归零、控制度/伤害/血量/平局各原因、未知和延迟结果；每帧握手确认并保存证据。
- [x] 完整Release构建、全部CTest、全部Python测试、现有14帧HUD回归；运行git diff --check。
- [x] 重启自建本地服务和新客户端，用agent-desktop实际操作客户端，复现用户截图数值并等待结算，截图确认红胜与正确统计；再查蓝胜、归零、平局和新局清理。
- [x] 自审每个结算入口/原因映射/字段缺失/累计比分/固定槽位/声音与刷新生命周期；记录未迁移机制和物理链路限制。
- [x] 更新记录、提交完成成果，创建独立完成标签并确认工作区干净。

## 完成记录

功能基线a6d7d51；视觉改动前标签rmul2027-pre-03-settlement-visual-20261113；完成标签rmul2027-step-03-done。Release构建与CTest30/30通过；Python50项中44通过、6项环境跳过；真实MQTT结算26帧与HUD14帧通过；原版动画、红蓝胜、归零、平局、换局及Esc/Ctrl+R已完成桌面复核。详细证据及完整规则模拟、其他旧页面、真实硬件和音频限制见docs/development/rmul2027-settlement.md。
