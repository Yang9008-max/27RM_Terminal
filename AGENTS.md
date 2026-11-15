# 项目协作约定

## Git 版本基线（用户明确要求）

- 在 `D:\MyTrain\projects\27RM_Terminal` 进行 2027 RMUL 3V3 开发。
- 每一步开始先检查当前分支、`git status` 和差异，保留用户已有工作。
- 每次重大改动前，先保存当前应纳入版本的文件为提交，并创建带说明的 Git 标签；工作区已干净时直接给现有提交打标签，无需空提交。
- 上述操作已获用户持续授权，正常的本地提交、标签和开发分支无需反复确认。
- 当前开发分支为 `rmul2027/develop`；跨多个模块或需保留替代方案时，可按阶段新建功能分支。
- 每一步完成后运行适合该改动的检查，将结果和限制记录到文档或提交说明，并提交该阶段成果。
- 不覆盖既有标签，不改写已保存的历史；切换旧版本优先使用独立分支或 worktree，避免丢失工作区内容。
- SDK、虚拟环境、编译产物和运行时私有配置不提交。官方 PDF 和答疑快照保留在外部资料目录；仓库保留来源索引与校验信息。
- 详细约定及初始标签见 [Git 工作流](docs/development/git-workflow.md)。

## 当前任务边界

第七阶段 `rmul2027/07-transport` 修复通信审查问题，基线标签 `rmul2027-pre-07-transport-20261115`。静态/动态/模块/经济按optional增量更新，明确键鼠零值释放；键鼠和CustomControl各自限75Hz，CustomControl最大60B，断开前归零且重连不重放。CustomByteBlock默认raw，旧MQTT视频须显式`RM_CUSTOM_DATA_FORMAT=h264`，旧状态须`robot-status-v1`。地图采用MapClickCmd/MapClickInfo及编号表#3标记类型/#6地图模式；旧操作图未校准，仅本地标记，下行仅发观测信号。未知模块灰色问号。Release独立构建于`build/windows-ucrt64-transport`（原exe正在运行），34项CTest最终验证通过、Python50项通过、独立MQTT测试通过。主窗口集成测试现已隔离到65534/65533/65532端口，无需停止本地3333模拟器。完整记录见`docs/development/rmul2027-transport.md`；真实裁判端/硬件、其他消息存在性和旧前哨语义尚未验收。

第六阶段 `rmul2027/06-tactical` 迁移独立战术页，基线标签 `rmul2027-pre-06-tactical-20261114`。`GameData::rmulTacticalData()` 复用带存在性的比赛快照，固定双方 1/3/7 三车；不回退到旧 TacticalMock、默认血量或六车分析结果。战术页显示控制度/总攻击伤害/三车剩余血量，保留 Ctrl+Y 相机布局与 M 展开；地图为按 2027 图 3-1/3-4 自绘的 12×8 结构示意，未接实时定位/占领。停止旧战术定时预告和回血猜金币复活播报接线。旧操作页小地图、长按 TAB、H 兑换与其他旧事件仍未迁移。验收和边界见 `docs/development/rmul2027-tactical.md`。

第五阶段 `rmul2027/05-local-settings` 修复 P 面板音量与视频限帧，基线标签 `rmul2027-pre-05-local-settings-20261114`，完成标签 `rmul2027-step-05-done`。提示音量统一短语音/击杀/结算/试听，音乐默认 50%；禁用音频和缺少音乐均显式显示，支持本次运行选择本地音乐及独立试听。视频限帧在实际呈现路径执行，保留最新尾帧并防止其他重绘绕过。Release/CTest 33 项通过，真实窗口已检查拖动、两类试听、文件选择和 144 FPS 选项。详细边界见 `docs/development/rmul2027-local-settings.md`。正常桌面验收不要使用旧 `tmp/desktop-acceptance-20261113/launch-client.ps1`，它会设置 `RM_DISABLE_AUDIO=1`；交付应清除该变量并重启。音源缺口仍按可选音效包处理，未验收实际听感和真实硬件。

第一阶段已实现并验收：2027控制度、1/3/4/7血量槽位和模拟器到终端状态链路。实现与验收记录见 `docs/development/rmul2027-data.md`；此阶段完成不代表其他2027消息、RMUL完整规则或界面已迁移。后续阶段仍须按上述Git流程先保存版本再改动。

第二阶段接入RMUL公共顶部栏：控制度、双方1/3/7血量和服务端比赛时钟；实现与验收入口见 `docs/development/rmul2027-hud.md`。当前功能分支为 `rmul2027/02-rmul-hud`。旧小地图、TAB、登录/兑换菜单、重装侧栏及通用模拟器的420秒规则引擎尚未迁移；顶部时钟如实显示接收值，不以300秒截断掩盖问题。

后续真实桌面验收与136高HUD布局修正位于 `rmul2027/02-operator-check`，记录见 `docs/development/desktop-acceptance-20261113.md`，完成标签 `rmul2027-operator-20261113-done`。已安装并使用agent-desktop及agent-desktop-windows技能；TAB长按、真实图传和整机链路仍未验收。

第三阶段位于 `rmul2027/03-settlement`：模拟器按RMUL控制度→攻击伤害→1/3/7血量→平局裁决；客户端仅消费GameStatus官方结果；结算面板替换为双方各1/3/7三车。模拟器战斗时钟已改为300秒，完整演示含5秒倒计时共305秒。旧复活公式、控制区/阵亡自动扣分和其他旧页面仍未迁移。验收入口 `tools/verify_rmul2027_data.py --settlement`，记录及规则边界见 `docs/development/rmul2027-settlement.md`。运行主窗口集成测试时应停止独立模拟器，避免旧测试自动连接本地3333端口而接收外部阶段广播。

第三阶段完成标签 `rmul2027-step-03-done`：复用原红/蓝胜利PNG动画，之后显示带机器人图标和动态对比条的3V3战报。结算跨操作页/战术页可见；Esc关闭后同局统计不重开，Ctrl+R重看真实结果。最终Release/CTest 30项通过，独立MQTT结算26帧及HUD14帧通过；Python 50项中44通过、6项环境跳过。真实桌面已确认红胜、蓝胜、归零、平局和新局清理；真实硬件与音频仍未验收。

第四阶段位于 `rmul2027/04-settings`，基线标签 `rmul2027-pre-04-settings-20261113`，完成标签 `rmul2027-step-04-done`。P面板限红蓝1/3/7，显示重装/步兵/哨兵；仅已登录步兵赛前可选1/2性能，哨兵只观测、无半自动或键鼠控制。Command/Sync/StaticStatus性能字段按2027 optional处理，接通回执并防止反馈回发；模拟器支持部分字段更新与角色/阶段检查。原来未接线的自定义UI和图传方向入口禁用，删除假温度/通道；准备阶段性能提示按角色和真实确认值显示。Release/CTest 31项通过，Python54项中48通过、6项环境跳过；独立MQTT性能收发通过，真实窗口已确认六角色、性能回执与战斗锁定。记录见 `docs/development/rmul2027-settings.md`。旧TAB/战术页阵容、H展开/兑换和其他旧Hero逻辑仍未迁移；真实裁判端和硬件未验收。
