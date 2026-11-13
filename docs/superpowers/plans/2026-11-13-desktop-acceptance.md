# Windows 操作手验收与紧凑 HUD 修正 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 使用真实 Windows 桌面操作完成终端验收，修正实际窗口中控制度被血量卡片覆盖的问题。

**Architecture:** 保留主窗口按屏幕高度分配顶部区域的规则，将HUD的170逻辑像素设计面在不足170高的视口内等比缩小。用现有QML测试补足136高的真实紧凑场景，验收在当前200%缩放桌面进行。

**Tech Stack:** agent-desktop 1.3.0、Qt Quick、QtTest、现有本地MQTT/Socket.IO模拟器。

## Global Constraints

- 基线为9039b7d，改动前标签`rmul2027-pre-operator-20261113`；分支`rmul2027/02-operator-check`。
- 只连接127.0.0.1:3333/8000；用户已授权实际鼠标键盘模拟，无需重复确认。
- 保留主窗口和其他旧功能；只修复本次发现的HUD高度布局。
- 技能从vercel-labs/agent-desktop提交947b051fb7a03a797ad12b9bb942d4ffc6f91735安装，两个技能均保留完整references。

### Task 1: 重现与修复

**Files:** Modify `tests/unit/test_rmul2027_hud.cpp`, `src/qml/TopInfoBar.qml`。

- [x] 扩展`rosterAndLayout_data()`增加`height`列和1280×136场景，加载时设置真实高度。
- [x] 在`rosterAndLayout()`中检查每个控制度Text的scene底边不超过同方机器人卡片scene顶边，以及阶段文字仍在视口内。
- [x] 运行新增测试，记录136高的重叠失败；原先170高场景应通过。
- [x] 给TopInfoBar增加设计面：`contentScale = Math.min(1, height / 170)`，内部宽度`width / contentScale`、高度170、左上为变换原点。原RowLayout进入该设计面，数据绑定保持原接口。
- [x] 运行HUD测试，确认170/136高都通过。

### Task 2: 真实操作、记录和版本

**Files:** Create `docs/development/desktop-acceptance-20261113.md`; Modify `docs/development/rmul2027-hud.md`。

- [x] 编译客户端、运行CTest，重新运行14帧MQTT→QML验收。
- [x] 用agent-desktop在真实终端按P、1、Enter登录红方1号；查看数据并切换蓝方3号，验证固定红蓝颜色与血量/控制度映射。
- [x] 用本机模拟器准备600/590、HP123、归零和暂停输入；实际窗口截图核对，不以内部状态代替可见结果。
- [x] 验证Ctrl+T切换及旧TAB面板现状；记录尚未迁移项，最后回到操作页。
- [x] 记录技能安装、操作步骤、截图、发现和限制；保存提交及独立验收标签。
