# 2027 RMUL 通信修复（阶段七）

分支：`rmul2027/07-transport`。基线：`8e05359e8dc249a679882e3d3798fe4b5c1b2d01`，标签 `rmul2027-pre-07-transport-20261115`。

## 依据与范围

依据[官方2027通信协议 V1.0.0（20261029）](https://bbs.robomaster.com/wiki/20204847/1969151?source=7)，原始PDF及哈希见[资料索引](../research/rmul2027/local-sources.json)。PDF页序：键鼠/自定义控制53，动态状态62–63，模块状态63–64，地图67–68，自定义下行69。参考[北航通信后端](https://github.com/BUAA-Transistor-alg/RM2027-CustomTerminal-CommunicationBackend)提交 `77829f261d98f2eecdd3ce1b27803b101c78a4e5` 的schema和校验边界；没有引入其代码或替换Qt通信框架。

本阶段修复已复现的问题，不声明全部2027消息、旧操作页或实机链路迁移完成。

## 改动

- `RobotStaticStatus`、`RobotDynamicStatus`、`RobotModuleStatus`、`GlobalLogisticsStatus` 恢复显式optional。缺失字段保留前值，显式零/false仍更新。静态field/alive状态按机器人合并，避免分包改变状态优先级；经济网络转换也保留缺失值。
- 模块明细不生成未知模块的离线告警；未知指示灯为灰色问号。切换角色、重置会话时清理模块缓存。
- 动态字段4改为`reserved`，不再写入底盘能量或功率；模块字段11改为`modular_42mm`安装权限，不作为激光模块在线状态。模拟器同步名称及显示。
- 键鼠七个字段全部显式写入，释放金样为`0800100018002000280030003800`。
- 键鼠和CustomControl各自使用单调时钟，发送间隔至少13,333,334ns，定时器向上取整。合并待发送的最新状态；鼠标位移增量累加，主动归零取消旧位移。CustomControl拒绝超过60B的数据；返回成功信号仅表示Paho接受发布，不代表赛事端确认执行。
- 待发释放帧由通信层定时器发送，不依赖UI继续产生输入。发布失败最多额外尝试3次；断开前在预算内完成归零，随后清空待发数据。正常输入不等待QoS完成；退出时允许最多4个调度间隔的有限等待。连接已丢失时无法保证对方收到释放，重连不重放旧动作。
- CustomByteBlock只接受合法Protobuf及不超过300B的data，完整发出`customDataPayloadReceived`。默认不推断视频或状态，也不剥离看似封装前缀的合法原始数据。
- 新增上行`MapClickCmd`和下行`MapClickInfo`；目标列表7B。下行只发观测信号，不反馈为上行。按官方编号表将标记类型写入#3（proto名mode），地图/敌方模式写入#6（proto名type）；字段名与中文表含义容易混淆，仍需实机核验。
- 旧操作页图片没有2027坐标标定，取消28×15米坐标发送，明确显示本地标记。收到的地图消息当前仅进入观测信号，不投影到旧图；完整地图显示仍待原点、方向和比例验证。独立战术页保持12×8结构示意用途。
- 官方订阅表移除重复Event、旧部署/空中支援和CustomRobotData，加入MapClickInfo。内部历史类型保留；旧MapClickInfoNotify封装不能经通用MQTT接口发布。

## 队内自定义格式

`RM_CUSTOM_DATA_FORMAT`仅影响CustomByteBlock，不影响官方UDP HEVC主图传：

| 值 | 行为 |
| --- | --- |
| 未设置或`raw` | 只交付原始data |
| `h264` | 原始data同时交给现有队内H264视频链 |
| `robot-status-v1` | 长度严格等于现有RobotCustomStatus结构时解析旧队内状态 |

旧工业相机或模拟器若使用MQTT H264，需要显式设置`h264`。这两个兼容格式是本队约定，不属于官方通用数据格式；机械臂反馈应定义独立版本、类型、长度和字节序后再接解析器。

## 验证入口

新增`TestRmul2027Transport`覆盖稀疏状态、显式释放、速率调度、取消和重试。真实本地Broker验证：

```powershell
sim/.venv/Scripts/python.exe tools/verify_rmul2027_transport.py --receiver build/windows-ucrt64-transport/tests/test_rmul2027_transport.exe
```

工具申请独立本地端口，清理自己启动的Broker，输出在`tmp/rmul2027-transport/`。主窗口集成测试固定使用65534/65533/65532端口，避开运行中的3333/3334服务。

原客户端正在运行，Windows锁定其exe；本次Release构建放在`build/windows-ucrt64-transport`，没有关闭原客户端或模拟器。启动新版：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -Command '. ./tools/windows/environment.ps1; Set-Location build/windows-ucrt64-transport; ./RoboMasterClient2025.exe'
```

## 验收记录

- 2026-11-15：独立目录Release构建完成，资源检查通过。原exe运行导致首次原目录链接失败，随后使用独立构建目录。
- 修改前新增回归明确复现三项失败：热量分包清零血量、静态分包清零最大血量、全零释放无字段存在性。修改后通过。
- 完整CTest最终34项通过（含隔离后的主窗口集成测试）。新增跨消息顺序回归验证：性能Sync更新后，仅含热量上限的StaticStatus不得重新应用缓存的旧性能值。CTest默认跳过需要独立Broker的子用例。
- 独立Mosquitto实链路全套通过，无跳过：`tmp/rmul2027-transport/20261115-224905-934420/result.json`及同目录`receiver.txt`。含明确释放、60/61B边界、7B目标列表、地图编号表映射、下行不回发、原始28B不误解析、显式H264、301B拒收、稀疏经济、退出归零与重连不重放。保留非MQTT旧UDP原始数据入口后，最终构建再次通过通信CTest和实链路验证。
- Python协议、RMUL数据、性能、结算与比赛引擎50项通过，日志`tmp/native-build/python-07.log`。Protobuf生成/运行时一致性检查通过（生成6.33.2、运行时6.33.6）。

真实赛事引擎、裁判系统、机器人和键鼠焦点变化的实机效果尚未验收；旧前哨语义、其他消息存在性和旧功能接线仍按各阶段边界处理。

## 2026-11-18 审查修复：存活缓存与会话动态状态

改动前基线：`4dd6767`，附注标签 `rmul2027-pre-07-state-review-20261118`。修复仍在 `rmul2027/07-transport` 分支，完成标签 `rmul2027-07-state-review-done-20261118`。

- `RobotDynamicStatus.current_health` 和有效的 `GlobalUnitStatus.robot_health` 更新同一份按机器人保存的有效存活状态。后续只包含 `field_state` 的静态分包不会重新应用血量更新前的旧阵亡/存活值。全局血量仍严格按双方1/3/4/7槽位解析。
- 按消息接收顺序合并显式值：后来明确下发的静态 `alive_state` 仍会生效；没有血量的热量分包不改变存活缓存。显式 `field_state=1` 始终保持 OFFLINE，直到收到恢复在场的字段。
- MQTT启动、断线以及角色切换已调用的 `resetMatchStatus()` 现在清理所有机器人的动态血量、热量、射速、缓冲能量、经验、弹量、脱战状态/倒计时和远程补给权限，并清理相关时间戳与本地热量覆盖窗口。重置性能选择时同步禁用底盘/锁定发射机构，连接标记归为未连接；逐机器人通知视图刷新。
- 清理后的血量0是失效后的占位值，状态设为 OFFLINE，不表示收到官方阵亡数据。身份、名称和静态上限保留。同一会话内仍保留缺失字段，显式0/false正常更新；重连后的热量分包不能恢复旧血量和补给权限。

验证记录：

- 修复前新增13个回归失败：8个红蓝/归零或复活/动态或全局血量交错用例，1个在场优先级用例，4个红蓝直接重置或切换角色用例。仅修复存活缓存后剩4个会话重置失败，随后全部通过。日志：`tmp/native-build/review-state-before.txt`、`review-state-after-liveness.txt`、`review-state-focused.txt`。
- Release完整构建、资源检查和34/34项CTest通过；完整测试日志：`build/windows-ucrt64-transport/Testing/Temporary/LastTest.log`。
- 独立本地Mosquitto实链路22通过、0失败、0跳过，包含断开前授权、断开清理、重连后热量分包不恢复旧授权/血量，以及原有发送预算、明确释放和地图/自定义数据边界。结果：`tmp/rmul2027-transport/20261118-133436-008177/result.json`，同目录 `receiver.txt`。
- `git diff --check` 通过。实现计划与回归步骤见 [审查修复计划](../superpowers/plans/2026-11-18-transport-state-review.md)。

本补丁按客户端接收顺序处理跨主题的显式状态，没有增加服务端采样时间排序。验证覆盖本机原生构建与本地MQTT，未替代真实裁判系统/机器人验收；其他消息及历史自定义字段的会话生命周期仍不属于这次两项修复的范围。
