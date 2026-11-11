# 第一阶段：2027 RMUL 比赛数据

## 已实现的链路

`StateManager → MQTTPublisher → 本地/官方 MQTT → NetworkManager → GameData`。

依据本地官方《RoboMaster 2027 机甲大师高校系列赛通信协议 V1.0.0（20261029）》§2.2.4（印刷 p54–55 / PDF p55–56）。原件路径、SHA-256 和研究结论见 [调研资料](../research/rmul2027/sources.md)。

本阶段新增 `GlobalUnitStatus.total_CP_ally = 15` 和 `total_CP_enemy = 16`，均为 `optional uint32`。C++ 和 Python 使用同一份 schema；C++ 在构建时生成，Python 生成文件纳入版本控制。

## 控制度状态契约

| GameData 属性 | 含义 |
| --- | --- |
| `redControlPoints` / `blueControlPoints` | 最近一次收到的红/蓝控制度，uint32 |
| `hasRedControlPoints` / `hasBlueControlPoints` | 当前连接/回合是否收到过该方控制度 |
| `controlPointsChanged()` | 值或有效性变化时通知，完整应用当前包后发出 |

终端机器人 ID 小于100时，ally 为红方、enemy 为蓝方；蓝方 ID 则相反。存储采用固定红/蓝视角，后续界面可以据此转换为己方/对方。

- 首次缺失：有效性为 false，数值占位0不代表实际0，也不预填600。
- 明确收到0：有效性为 true，0会覆盖之前的正数。
- 后续包缺失某字段：保持该方最后已知值和有效性，另一侧可独立更新。
- 切换连接机器人、开启新 MQTT 会话、MQTT 断连或进入新回合：失效全局快照，等待新数据。
- 有效性指“当前会话/回合曾收到”，本阶段没有增加按秒过期的策略；不能把最后已知值当作持续连接或实时新鲜度证明。

## 血量和弹量槽位

| 槽位 | 红方终端视角 | 蓝方终端视角 |
| --- | --- | --- |
| 0–3 | 1、3、4、7 | 101、103、104、107 |
| 4–7 | 101、103、104、107 | 1、3、4、7 |

血量必须是八项，4号槽位保留。RMUL 3V3 未配置的4/104号由模拟输入填0；不会把第4个槽位的哨兵映射成旧阵容的工程或空中机器人。模拟器默认 ID 已使用以上八槽顺序；其旧比赛引擎仍不是完整的2027 RMUL规则实现。

数组中的0是有效血量，会覆盖已连接机器人的旧血量，并保留 `RobotData.hasTabGlobalSnapshot=true`。Tab/状态数据访问器也优先使用有效全局快照的0。空数组按“此次未下发”处理，保持上次快照；非空但长度不是8，或值超出内部 uint16 表示范围时，拒绝整组血量并记录日志，不猜测其他阵容。畸形血量不阻止同包合法控制度更新。

`robot_bullets` 使用己方四槽顺序，可独立于血量更新；空数组不清空弹量，非四项数组拒绝。1号槽进入现有42mm弹量字段，其余进入17mm字段；重装安装42mm后的完整弹种与业务模型仍留待后续开发。血量总和目前是协议中每方四槽的和，正式3V3应确保4号槽为0。

新回合或断线只清除全局快照有效性和总和，不清空其他独立来源的静态/动态机器人数据。血量消费者应同时检查快照有效性。

## 模拟器控制接口

```python
manager.set_control_points("red", 600)
manager.set_control_points("blue", 0)     # 明确发送0
manager.set_control_points("blue", None)  # 不发送该字段
manager.set_robot_hp(manager.robot_ids.index(3), 123)
```

Web 后端接收 `{"command":"set_control_points","team":"red","value":600}`；`value:null` 表示不下发。team只接受red/blue，数值只接受0..4294967295的整数。现有 `set_hp` 命令的 `robot_id` 参数沿用数组索引，不能直接把机器人编号3作为步兵索引；正式界面后续再统一命名。

## 重现验收

在仓库根目录运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/windows/build.ps1 -Test
sim/.venv/Scripts/python.exe tools/verify_rmul2027_data.py
```

联调脚本会启动仅监听127.0.0.1随机端口的临时Mosquitto，分别以红1号和蓝103号终端运行Qt接收测试。每个视角7帧，接收端完成每帧状态校验后才允许发布器推进下一帧；覆盖首包未知、血量变化和归零、控制度归零、后续缺失保持、稀疏包单侧更新，并检查断连后的有效性。发送由真实模拟器发布入口完成，接收经过正式NetworkManager和GameData，不在测试中替换消息解析逻辑。

每次联调的输入、Qt日志、Broker日志和 `result.json` 保存在 `tmp/rmul2027-data/<运行时间>/`。普通CTest中的联调用例在缺少显式环境参数时跳过，必须运行上面的独立脚本才能声称完成真实MQTT联调。

模拟器单元测试在 `sim/` 目录运行：

```powershell
.venv/Scripts/python.exe -m unittest tests.test_rmul2027_data tests.test_protocol_contract tests.test_match_simulation_engine
```

Windows重新生成Python协议：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/windows/generate-sim-protobuf.ps1
```

该入口使用官方protoc 33.2，生成Python 6.33.2代码，与当前6.33.6运行时兼容；C++继续使用原生SDK配套的编译器。[官方下载及SHA-256](https://github.com/protocolbuffers/protobuf/releases/expanded_assets/v33.2)由脚本固定核验。

## 迁移边界

本阶段完成控制度、固定血量/弹量槽位和GlobalUnitStatus标量presence处理。旧前哨字段名称/语义、其他消息、命令权限、地图坐标、300秒赛制、完整RMUL比赛引擎、机械臂子协议和新界面仍需要后续迁移。旧UDP `0x0003` 转换器生成的裁剪血量数组不符合新MQTT八槽契约，不能用它替代本阶段验收链路。

没有使用2027官方硬件抓包或实车验收。实机若下发不同的数组形态，需要先确认官方契约与报文证据，再显式适配；不恢复按长度猜阵容。

## 本机验收记录（2026-11-11）

- 完整Release客户端和测试构建成功，CTest **28/28通过**，包括既有GameData、界面和协议测试。
- 模拟器数据、协议合约和比赛引擎测试 **30项通过**；Node地图几何测试 **3项通过**。
- 新用例先在旧实现上复现红/蓝血量错位、已连接机器人0血量被忽略、缺失包清除快照；修正后通过。
- 本地Mosquitto真实链路：红1号和蓝103号各 **7帧，共14帧通过**；由Qt接收测试直接读取GameData校验，包含断连失效检查。
- Python生成代码6.33.2与运行时6.33.6、权威schema描述符一致性检查通过。
- 最终联调证据目录：`tmp/rmul2027-data/20261111-224332-711684/`，含输入帧、逐帧回执、接收日志、模拟器日志、Broker日志及`result.json`。
- Git改动前标签：`rmul2027-pre-01-2027-data`；工作分支：`rmul2027/01-2027-data`；阶段完成标签：`rmul2027-step-01-done`。
