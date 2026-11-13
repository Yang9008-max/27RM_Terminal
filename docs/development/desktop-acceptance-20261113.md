# Windows 真实桌面操作验收（2026-11-13）

已用 agent-desktop 实际向运行中的 Qt 客户端发送键盘、鼠标输入并逐次截图复核。红方1号、蓝方3号身份切换，控制度600/590、血量差异及归零显示通过；发现并修正高 DPI 紧凑窗口的顶部信息重叠。本次是本机模拟器验收，不代表官方设备或整机验收完成。

## 安装与使用的技能

来源：[vercel-labs/agent-desktop](https://github.com/vercel-labs/agent-desktop)，固定提交 `947b051fb7a03a797ad12b9bb942d4ffc6f91735`。

- `C:\Users\Administrator\.codex\skills\agent-desktop\SKILL.md`
- `C:\Users\Administrator\.codex\skills\agent-desktop-windows\SKILL.md`

两个技能及各自 references 均已安装，本轮已读取使用。通过 skill-installer 的 GitHub 安装脚本先安装到项目忽略目录，再用 PowerShell 复制到技能库；没有修改系统 ACL。安装并验证 CLI `agent-desktop@1.3.0`，全局入口为 `C:\Users\Administrator\AppData\Roaming\npm\agent-desktop.cmd`。使用 CLI，无需添加 MCP 配置；这不等于安装了 windows-mcp。

`status` 返回 Windows、版本1.3.0、accessibility granted；automation/screen_recording 为 not_required。实际使用了窗口枚举、UIA 快照/查找、截图、物理按键和物理鼠标输入。GUI 操作遵守观察→操作→复核，不以 delivered_unverified 当作验收通过。

## 环境与输入

- 客户端：`build/windows-ucrt64/RoboMasterClient2025.exe`，窗口标题仍为旧的 `RM26 Custom Client`。
- 屏幕2560×1600物理像素，200%缩放；主窗口实际给顶部栏分配136逻辑像素高度。
- 仅本地：MQTT `127.0.0.1:3333`，模拟器网页/Socket.IO `127.0.0.1:8000`。
- 本地 Socket.IO 通过现有 simple_websocket 发送 referee_command 并等待 ACK，修改模拟器状态，再由原有发布链路更新客户端；没有直接写 GUI 显示值。
- 身份切换时同时修改模拟器 current_robot_id，使其按对应视角发布全局槽位，不能只改 GUI 登录身份。
- 正常血量槽位输入：红1/3/4/7为500/123/0/300，蓝1/3/4/7为450/350/0/250；4号仅占协议槽位。

## 实际操作与观察

| 操作 | 可见结果 | 证据文件 |
| --- | --- | --- |
| Ctrl+T从旧战术页进入操作页 | 显示新RMUL公共顶部栏 | 02-operator-view.png（修复前） |
| P、1、Enter | 设置面板显示当前机器人R1-Hero，登录按钮变退出 | 05-logged-in.png、09-fixed-login.png |
| 设置面板中8、Enter；模拟器切到103 | 当前机器人B3-Standard；顶部仍固定左红右蓝，红3为123、蓝3为350 | 10-blue-login.png |
| 恢复比赛 | 暂停遮罩消失，时钟正常递减，600/590及六车血量可读 | 12-blue-normal.png |
| 双方控制度归零、红3及蓝7血量归零 | 控制度均显示0，红3与蓝7显示HP 0，其余对应值不串位 | 13-zero.png |
| Ctrl+T切战术页，再切回 | 页面切换有效；战术页仍是旧阵容/地图 | 14-tactical.png、15-return-operator.png |
| 轻按TAB | 释放后的截图仍为操作页 | 16-tab-tap.png |
| 设置面板选1并Enter，模拟器恢复红1与正常数值，恢复比赛，Esc关闭面板 | 当前机器人R1-Hero，恢复正常公共HUD，设置面板关闭 | 19-red-restored.png、22-after-escape.png |

截图均在本机 `tmp/desktop-acceptance-20261113/`，忽略目录不纳入Git。`11-role-menu.png` 只证明一次物理点击后面板消失，不能证明下拉菜单打开；命名不作为结果依据。UIA 对该Qt窗口的同类QQuickWidget区分有限，后续一次引用点击因 receives_events 被group遮挡而拒绝，未绕过检测。角色切换已通过真实键盘及截图确认。设置面板关闭过程中P及外部点击的结果不一致，最终Esc关闭成功；不据此宣称所有鼠标/焦点路径均已通过。

TAB为按住显示、松开隐藏的旧功能，CLI明确不支持key-down/key-up；本轮只验证轻按及释放后的状态，**未完成长按面板验收**。不能把旧TAB页面认定为RMUL适配完成。

## 重叠问题及修复

原HUD按170逻辑像素设计，但主窗口在本机仅分配136；卡片从底边定位后挡住控制度。先给原有测试增加1280×136及控制度底边不超过卡片顶边的断言，修复前该场景失败，原1920×170、1280×170均通过。失败日志保存在 `tmp/desktop-acceptance-20261113/layout-test.txt`。

`TopInfoBar.qml` 新增170高设计面，按 `min(1, height / 170)` 等比缩放，并反算内部宽度。没有改变数据接口。重建后的08、10、12、13、22截图均能同时读出控制度和六车血量。

## 验证与版本

- Release完整构建成功；`tools/windows/build.ps1 -Test`：CTest **29/29通过**（约45.69秒）。
- 紧凑高度布局回归由失败转为通过，涵盖170/136高和1280/1920宽组合。
- `tools/verify_rmul2027_data.py --hud`：红1与蓝103视角各7帧，**共14帧通过**。包含未知、归零、后续缺失保留、会话清理等自动验收项；这些自动项与本轮GUI人工式操作记录分别计数。
- MQTT→QML证据：`tmp/rmul2027-data/20261113-125142-993052/result.json`。
- 改动前：`9039b7d`，标签 `rmul2027-pre-operator-20261113`。
- 本轮分支：`rmul2027/02-operator-check`；完成标签 `rmul2027-operator-20261113-done`。

结束时客户端和本地模拟器保留运行，红1身份、正常血量、控制度600/590，位于操作页。恢复后的比赛倒计时会继续递减；截图表示当时状态，不是永久冻结值。临时启动/输入脚本、PID及服务日志保存在同一tmp目录。

## 待迁移与验收边界

旧战术页、地图、TAB、登录角色命名、兑换面板仍含RMUC内容；系统消息仍产生空中支援、金币等旧规则提示。顶部“重装”不代表旧Hero内部类型及侧栏已迁移。鼠标/焦点交互需要后续专门复查。本机未连接真实图传、裁判硬件或机械臂，画面黑底及“未检测到图传”符合当前环境。赛制与模拟器规则引擎统一仍是后续工作，不在本次布局修复中扩展。
