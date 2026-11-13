# 2027 RMUL 3V3 公共作战信息栏

本阶段把第一阶段的控制度与血量状态接入实际主窗口顶部栏。复旦 Qt 框架、图传和主窗口入口继续复用，顶部信息组织改为 RMUL 3V3。

## 界面与数据契约

| 显示项 | 数据来源 | 未收到数据 | 明确收到 0 |
| --- | --- | --- | --- |
| 红、蓝控制度 | GameData 的 red/blueControlPoints 与 hasRed/BlueControlPoints | `—` | `0` |
| 双方重装1、步兵3、哨兵7血量 | GlobalUnitStatus 的固定槽位快照 | `HP —` | `HP 0` |
| 阶段、局数、暂停、时钟 | 比赛状态统一接收接口，hasMatchStatus 标识有效性 | 等待比赛数据 / `--:--` | `00:00` |

控制度使用完整 uint32 数值，4294967295 不会被 QML int 截断为负数。卡片只显示已收到的全局血量，不凭客户端连接态隐藏合法血量，也不凭血量0推演离线、判负或地图控制区归属。

后续缺失字段保留之前已收到的值；切换机器人身份、MQTT会话开始或断开后清除有效性，卡片和控制度回到未知。新局的全局快照重置仍沿用第一阶段逻辑。没有独立超时失效机制，显示值代表本会话最后收到的快照，不等价于链路在线指示。

界面标明“3V3 · 5 分钟制”。实际时钟完全使用服务端倒计时：300显示05:00，0显示00:00；收到420仍显示07:00，不修改接收值掩盖对端配置问题。底层旧默认420不会作为尚未收到数据时的顶部时钟。

本阶段不显示旧基地、前哨站、默认队名/队徽、默认经济或未经接通的占领进度。UI用“红方”“蓝方”标识阵营。

## 实现位置

- `src/qml/TopInfoBar.qml`：主窗口现有加载入口；中央赛制、时钟和阶段。
- `src/qml/RmulTeamPanel.qml`：单方控制度和三车布局。
- `src/qml/RmulRobotCard.qml`：机器人编号、角色和全局血量；显式接收GameData。
- `GameData::getRobotInfo()`：新增hasGlobalHealth/globalHealth；`resetGlobalUnitStatus()`逐卡通知，避免50ms dataChanged节流跳过清理。
- `GameData::hasMatchStatus/resetMatchStatus()`与`NetworkManager`：比赛状态接收有效性和会话清理。
- `tests/unit/test_rmul2027_hud.cpp`：实际QML文字、布局和MQTT接收验收。

## 一键验收

在 `D:\MyTrain\projects\27RM_Terminal` 执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/windows/build.ps1 -Test
sim/.venv/Scripts/python.exe tools/verify_rmul2027_data.py --hud
```

第二条自动启动仅监听127.0.0.1的临时Mosquitto，使用模拟器生产发布接口和终端NetworkManager，把数据送入实际QML组件。程序结束后关闭自建进程，不改电脑网卡和现有模拟器配置。正常输出包含：

```text
PASS perspective=1, frames=7
PASS perspective=103, frames=7
Verification passed: ...\result.json
```

结果目录为 `tmp/rmul2027-data/<时间戳>/`。每方保存3张实际渲染截图：`hud-1-frame0.png`（控制度未知）、`hud-1-frame1.png`（600/590）、`hud-1-frame3.png`（控制度与部分血量归零）；蓝方对应`hud-103-*`。`receiver-*.txt`记录每帧断言，`fixture-*.json`保存输入和预期，`result.json`标明验证目标为QML HUD。

自动验收覆盖红蓝视角、1/3/7映射、4号协议占位不显示、控制度/血量归零、后续缺失保留、300秒与0秒、暂停、断线清理。Qt组件测试另覆盖4294967295、同队换身份、1280/1920宽度边界。普通CTest中的live项会跳过，必须单独运行上面的`--hud`命令。

原第一阶段验收仍可运行：

```powershell
sim/.venv/Scripts/python.exe tools/verify_rmul2027_data.py
```

构建后双击根目录 `start-client.cmd`。如果启动后处在旧战术大地图页面，按 `Ctrl+T` 切回操作界面，顶部即为新的RMUL信息栏；再次按键会切回尚未迁移的旧战术页。

## 当前边界与后续迁移

后续第三阶段已经迁移判胜与结算页，并将模拟器正式战斗时钟统一为300秒（完整演示含倒计时305秒）；因此下文记录的420秒时钟边界只适用于本阶段原始提交。当前实现、验收入口和仍未迁移的完整规则见[结算迁移记录](rmul2027-settlement.md)。旧TAB页面仍未迁移，不能与新版结算页混同。

这是公共顶部栏的交付，不代表完整RMUL作战终端已经完成。旧小地图/TAB统计/登录角色/兑换菜单/战术页面仍需迁移，重装机械臂和42mm安装侧栏尚未交付。不要因顶部已有“重装”字样而把旧HERO内部类型、旧英雄功能认定为重装适配完成。

通用网页模拟器的比赛流程、经济和规则提示仍包含RMUC的420秒逻辑。本次验收器为HUD显式发送300秒场景，没有改写整个模拟器规则引擎；正常网页模拟器若发送420，顶部会如实显示07:00。下一阶段应统一RMUL赛制配置、模拟器与功能门控，再推进地图和角色侧栏。

运行HUD验收时，旧GameData的能量机关提示模块可能记录`RuneVoice invalid_remaining`警告；它仍属于尚未迁移的旧规则逻辑，不是HUD数据被拒绝。物理2027官方设备、视频融合后的整机运行和赛场通信均需另行验收。

依据是项目已归档的RMUL规则V1.0.0（20261015），PDF17的阵容和五分钟赛制、PDF37–38的控制区/控制度机制；来源与哈希见[调研来源](../research/rmul2027/sources.md)。2026-11-13尝试读取官方动态页面，网页工具未返回正文，本阶段没有据此宣称规则更新或改变规则基线。

## 本机验证记录（2026-11-13）

- Release客户端及全部测试构建成功，CTest **29/29通过**（约49.5秒）。
- 新增HUD Qt测试通过（包含实际QML组件与1280/1920布局）；实现前同一测试在旧界面失败。
- 本地MQTT → NetworkManager → GameData → QML，红蓝双方共14帧通过。
- 已检查实际渲染的正常数据截图，红蓝颜色、控制度、六张卡片和时钟文字均可读。
- 首轮证据：`tmp/rmul2027-data/20261113-122709-798458/result.json`。
- 原第一阶段的GameData验收也重新运行，红蓝双方14帧通过；证据：`tmp/rmul2027-data/20261113-122943-070359/result.json`。

改动前基线标签为`rmul2027-pre-02-rmul-hud`，指向第一阶段完成提交`8d28c3f`；开发分支为`rmul2027/02-rmul-hud`。阶段完成后保存提交及`rmul2027-step-02-done`标签。截图和测试运行产物保留在本机tmp中，不纳入Git。

### 真实桌面补充验收

随后使用agent-desktop在2560×1600、200%缩放的Windows桌面实际登录红1、切换蓝3、改变模拟器血量和控制度、检查归零与页面切换。发现主窗口实际分配136逻辑像素时，原170高HUD的卡片遮住控制度；已通过设计面等比缩放修复，并新增1280×136回归断言。完整构建和CTest29/29、红蓝14帧MQTT→QML重新通过。实际截图、技能安装、鼠标/焦点路径限制及TAB长按未验收情况见[桌面操作报告](desktop-acceptance-20261113.md)。
