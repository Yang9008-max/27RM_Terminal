# 2027 通信迁移初步审计

日期：2026-11-11。对照版本见 [来源](sources.md)。本文是研究差异清单，不是已完成适配声明。

## 可以沿用的传输基础

2027 协议印刷 p46（PDF47）仍定义：MQTT 携带 Protobuf 二进制，Topic 为消息名，服务器 `192.168.12.1:3333`，终端 `192.168.12.2`，MQTT Client ID 填连接的机器人编号。

主图传仍走 UDP3334：1920×1080、60fps、HEVC/H.265；8B 大端头包含帧号 uint16、片号 uint16、整帧大小 uint32；每包最大 1400B，数据片最大 1392B，无重传。复旦已有相同形态的 `VideoReceiver` 和 FFmpeg 解码链，优先保留后用实际设备验证分片、丢包恢复、解码和延迟。

因此“换成 2027 通信”主要涉及协议契约、字段语义、路由与状态适配，不必重写整个 MQTT/视频底层。

## 必须处理的差异

| 项目 | 官方 2027 证据（印刷页 / PDF页） | 当前实现与影响 | 迁移要求 |
| --- | --- | --- | --- |
| 控制度 | p55/PDF56：`GlobalUnitStatus` #15 `total_CP_ally`，#16 `total_CP_enemy` | 当前 schema 无此两项，`GameData::updateGlobalUnitStatus` 以基地/前哨站为中心 | 新增协议字段和 RMUL 控制度模型，再接顶部 UI；未知与真实 0 分开 |
| 全队血量顺序 | p54/PDF55：己方1/3/4/7，对方1/3/4/7 | `GameData.cpp:2621` 按长度猜 1/2/3、1/2/3/4/7 或扩展阵容。8 项数据会落入旧 compact 分支，第二项本应3号却会映射2号 | 按 2027 明确槽位解码，再按 RMUL 可见阵容过滤。先核验实际 RMUL 是固定8槽还是裁剪；不能先删4号槽 |
| R/B 前哨字段 | p54–55/PDF55–56：#4/#5 是R前哨，#9/#10是B前哨 | 当前对应“己方/对方前哨”，蓝方视角会有语义风险 | 通用协议保留固定红/蓝含义；RMUL 界面不显示前哨 |
| 大量显式 presence | p52–74/PDF53–75：大部分标量/bytes 标 `optional` | 旧 schema 多为 implicit；部分接收代码对缺失字段取默认0并更新状态 | 同步生成 C++/Python；使用 has_* 区分未下发与0。零热量、0血量、false、operation=0均有实际含义 |
| 动态 #4 | p61/PDF62：`reserved` 保留位 | 当前名 `current_chassis_energy` | 不再将该位解释为实测底盘能量；旧能量组件须明确来源 |
| 模块 #11 | p62–63/PDF63–64：`modular_42mm` 表示**安装权限** | 当前为 `laser_detection_module` | 移除旧激光状态解释；权限与已安装分别建模。`Event` #16 是安装成功事件（p57/PDF58），不是持久状态快照 |
| 地图方向与格式 | p66–67/PDF67–68：下行 `MapClickInfo`，上行 `MapClickCmd`，各8字段；表格#3为标记类型、#6为地图/机器人标记模式 | 当前 `MapClickInfoNotify` 也是8字段，但发送函数将#3填为地图/机器人模式、#6填为攻击/防御等类型；订阅表未纳入新 `MapClickInfo` | 同时改上下行路由和字段语义，不只改 Topic 名。核对#3/#6取值；收到标记不能再次触发发送；目标列表按固定7B约定编码 |
| 旧指令与订阅 | 2027 §2.1 共32个Topic，未列旧英雄部署和空中支援Topic | 订阅含 `DeployModeStatusSync`、`AirSupportStatusSync`、`CustomRobotData`，还有重复 `Event`；发送函数保留旧功能 | 先维护协议契约，再按 RMUL 角色/阶段授权功能。不能把“协议有该Topic”视为“RMUL允许该动作” |
| 自定义上下行 | p7、38、47、49、68/PDF8、39、48、50、69 | `sendData` 封装CustomControl但未见60B检查/75Hz限速；接收CustomByteBlock默认转视频并按长度猜旧状态结构 | 增加长度/速率边界与明确的队内子协议；主视频、机械臂反馈和旧自定义状态不可按长度混判 |

注意：字段更名、optional 增加不一定改变普通非零值的编码，所以“能解析”不等于“语义和缺失值处理正确”。`GameStatus`、`Buff`、`AssemblyCommand`、`SentryStatusSync` 的字段声明初比一致，也仍需核验语义和应用行为。

## 小机械臂与自定义回传的边界

| 链路 | 官方上限 | 应用层建议 |
| --- | --- | --- |
| 终端→机器人：`CustomControl.data` → UART `0x0311` | 60B，75Hz，MQTT最高QoS1 | 在Qt侧做本地输入适配、明确字节序/版本/序号；只发送最新控制状态，断连不重放积压的旧动作 |
| 机器人→终端：UART `0x0310` → `CustomByteBlock.data` | 300B（2.4kbit），50Hz，MQTT最高QoS1 | 使用有标识的队内协议，承载机械臂状态、故障和允许获得的规则数据；长度限制针对 data，不是含Protobuf头的整个MQTT payload |

300B×50Hz 理论应用载荷为 15,000B/s≈120kbit/s，尚未计额外开销与丢包。不能把仓库中模拟器能灌入的工业相机码率当作真实官方链路能力。主图传 UDP 不占这个 300B 数据通道。

当前键鼠发送最小间隔写作13ms（`NetworkManager.cpp:570`），理论可达76.9Hz；中立帧又绕过该节流。迁移时应审查其与75Hz上限的关系，同时保留及时释放按键的语义。本轮未改实现，也未测得官方限频后的行为。

`CustomByteBlock` 当前解析见 `NetworkManager.cpp:1522`：所有非空数据先发往视频信号，28B以上且不超过100B的数据再按旧 `RobotCustomStatus` 拷贝。这不适合作为任意机械臂反馈协议，需先设计分流与验证，再接入业务。

## RMUL 数据缺口与官方示例歧义

1. **三个控制区的状态来源。** UART `0x0101` 第二个32位字的 bit16–27 已定义 R/B高地、中央控制区的激活/控制状态（印刷p11/PDF12）。MQTT §2.2 已确认控制度 #15/#16，但未找到这三个控制区逐区状态的明确字段；`GlobalSpecialMechanism`/`Event` 的对应枚举描述仍主要是 RMUC。可讨论机器人读取0x0101后，经0x0310合法回传；须核验链路实际下发、哨兵限制及队内数据定义。不能仅凭总控制度反推每区状态。协议此处也未给每区完整0–5占领进度，UI不能显示未经来源证明的精确进度。
2. **雷达示例不能直接编译。** 印刷p68/PDF69 原文写 `repeated uint32 RadarSingleRobotInfo;`，缺字段编号且与其后子结构不一致；已用PDF图像确认并非OCR错误。RMUL 3V3无雷达机器人阵容，先不依赖此消息；将来需要时向官方求证线格式，不能擅自把旧格式标为已核验。
3. **旧字段名与表义不一致。** `GlobalUnitStatus` 的 #3/#8 仍叫 `base_shield`，2027表格却描述基地阵地控制状态（PDF55–56）。RMUL不展示该字段；通用2027适配若保留，需要记录歧义并以官方澄清/设备证据确认。
4. **RMUL装配不等于RMUC科技核心装配。** `AssemblyCommand` 与 `TechCoreMotionStateSync` 描述科技核心流程；RMUL规则描述工作台安装模块化42mm。不可把旧工程兑换按钮改名成“安装42mm”就直接发送该命令。

## 代码改动范围，供后续方案讨论

- 协议源：`src/network/proto/robomaster.proto`、`protocol_manifest.json`；继续保持一份权威schema。同步生成C++构建产物与 `sim/robomaster_pb2.py`，更新协议验证工具/测试基线。
- 路由与发送：`src/network/NetworkManager.cpp`、`MqttManager.*`；维护订阅/发布方向、长度/频率、角色和阶段权限。
- 领域状态：`src/core/GameData.*`、`GameConstants.h`、`MapCoordinateMapper.*`；定义RMUL角色、控制度、数据有效性与坐标转换。
- 模拟器：`sim/server/mqtt_publisher.py`、`state_manager.py`、`match_simulation_engine.py`；同步使用2027协议，并提供RMUL场景和故障输入。
- 验证重点：显式0与缺失、固定槽位及蓝方视角、地图只单向发送、300/60B边界、断线/过期状态、机械臂失联行为、实际2027设备收发和1080p60图传。

下面是自动提取后人工补充适用范围的研究表。发送频率来自官方概览，不表示所有Topic都要在RMUL启用；“不启用”是本次建议。字段差异数按同名消息对比，不包含枚举语义、数组顺序或运行时行为。新地图消息与旧MapClickInfoNotify名称不同，表中记8项差异，不表示线格式凭空增加了8个字段。

<!-- TOPIC_TABLE -->

| §2.2 | Topic | 方向 | 官方频率/上限 | 字段差异数 | RMUL用途/建议 | PDF起始页 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | `KeyboardMouseControl` | 终端→机器人 | 75Hz | 7 | 保留键鼠输入，确认来源选择 | 53 |
| 2 | `CustomControl` | 终端→机器人 | 75Hz | 1 | 机械臂等队内控制，60B | 53 |
| 3 | `GameStatus` | 服务器→终端 | 5Hz | 0 | 比赛阶段、倒计时、结算 | 54 |
| 4 | `GlobalUnitStatus` | 服务器→终端 | 1Hz | 14 | 新增控制度；固定阵容槽位 | 56 |
| 5 | `GlobalLogisticsStatus` | 服务器→终端 | 1Hz | 4 | 金币；过滤非RMUL科技等级 | 57 |
| 6 | `GlobalSpecialMechanism` | 服务器→终端 | 1Hz | 0 | 枚举主要为RMUC；不据此推断3V3控制区 | 57 |
| 7 | `Event` | 服务器→终端 | 触发 | 2 | 筛选RMUL事件；含安装成功16 | 58 |
| 8 | `RobotInjuryStat` | 服务器→终端 | 1Hz | 10 | 伤害统计；不显示飞镖等非适用项 | 59 |
| 9 | `RobotRespawnStatus` | 服务器→终端 | 1Hz | 6 | 复活展示；动作按RMUL规则限制 | 60 |
| 10 | `RobotStaticStatus` | 服务器→终端 | 1Hz | 14 | 当前连接机器人身份与上限 | 61 |
| 11 | `RobotDynamicStatus` | 服务器→终端 | 10Hz | 13 | 当前机器人动态状态；#4为保留位 | 62 |
| 12 | `RobotModuleStatus` | 服务器→终端 | 1Hz | 11 | 模块状态与42mm安装权限 | 63 |
| 13 | `RobotPosition` | 服务器→终端 | 1Hz | 5 | 定位可用时展示；坐标须核验 | 64 |
| 14 | `Buff` | 服务器→终端 | 获得时触发，后1Hz | 0 | 增益及到期 | 65 |
| 15 | `PenaltyInfo` | 服务器→终端 | 触发 | 3 | 判罚展示 | 66 |
| 16 | `RobotPathPlanInfo` | 服务器→终端 | 触发 | 4 | 仅展示允许获得的自动哨兵轨迹 | 66 |
| 17 | `MapClickInfo` | 服务器→终端 | 触发，限频见§1.3.1 | 8 | 新下行地图标记；实际RMUL权限待核验 | 67 |
| 18 | `MapClickCmd` | 终端→服务器 | 触发，限频见§1.3.1 | 8 | 新上行标记；初版建议先做本地标注 | 68 |
| 19 | `RadarInfoToClient` | 服务器→终端 | 触发 | 无法判定 | RMUL初版不依赖；官方示例有歧义 | 69 |
| 20 | `CustomByteBlock` | 机器人→终端 | 50Hz | 1 | 回传状态；300B，自定义子协议 | 69 |
| 21 | `AssemblyCommand` | 终端→服务器 | 触发≤10Hz | 0 | RMUC科技核心流程；RMUL初版不启用 | 69 |
| 22 | `TechCoreMotionStateSync` | 服务器→终端 | 1Hz | 8 | RMUC科技核心流程；RMUL初版不启用 | 70 |
| 23 | `RobotPerformanceSelectionCommand` | 终端→服务器 | 触发≤10Hz | 3 | 步兵赛前体系选择；不开放哨兵半自动 | 71 |
| 24 | `RobotPerformanceSelectionSync` | 服务器→终端 | 1Hz | 3 | 按角色投影性能选择状态 | 71 |
| 25 | `CommonCommand` | 终端→服务器 | 触发≤10Hz | 2 | 初版仅按补给区条件开放重装/步兵弹量兑换 | 72 |
| 26 | `RuneActivateCommand` | 终端→服务器 | 触发≤10Hz | 1 | RMUL不启用 | 72 |
| 27 | `RuneStatusSync` | 服务器→终端 | 1Hz | 3 | RMUL不启用 | 73 |
| 28 | `SentryStatusSync` | 服务器→终端 | 1Hz | 0 | 收到后可观测，不据此开放哨兵操控 | 73 |
| 29 | `DartCommand` | 终端→服务器 | 触发≤10Hz | 3 | RMUL不启用 | 73 |
| 30 | `DartSelectTargetStatusSync` | 服务器→终端 | 1Hz | 2 | RMUL不启用 | 74 |
| 31 | `SentryCtrlCommand` | 终端→服务器 | 触发≤10Hz | 1 | RMUL哨兵全自动，初版不开放 | 75 |
| 32 | `SentryCtrlResult` | 服务器→终端 | 触发 | 2 | 初版不依赖哨兵指令回执 | 75 |

另有 `RadarSingleRobotInfo` 子消息，3个标量从隐式字段变为optional；已纳入JSON逐字段记录。PDF起始页是消息声明的起始页，字段说明有时在前一页。
