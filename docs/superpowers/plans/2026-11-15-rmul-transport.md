# 2027 RMUL 通信修复实施计划

**目标：** 修复审查确认的字段缺失覆盖、输入释放、控制预算、地图通信与自定义回传误解析问题。

**架构：** 保留现有 Qt、GameData、MQTT 和 UDP 图传结构。以官方2027协议为依据修改字段与更新边界；控制发送由单调时钟和最新状态调度；地图业务仅开放有依据的功能。

**技术：** C++17、Qt6、Protobuf、Paho MQTT C、Python模拟器、CTest。

## 约束

- 开发分支 `rmul2027/07-transport`，基线标签 `rmul2027-pre-07-transport-20261115`。
- 保留用户现有版本；不操作真实机器人或赛事引擎，不停止当前模拟器。
- 官方PDF与参考仓库保留在外部资料目录；生成的Python协议与schema同步。
- 使用项目内 `tmp/native-build` 作为原生工具与测试临时目录。
- 地图字段3/6存在官方文档歧义，采用官方proto字段号并记录设备验证限制；接收标记不得反馈为发送。

## 任务一：字段存在性与状态更新

文件：`src/network/proto/robomaster.proto`、`sim/robomaster_pb2.py`、`src/core/GameData.cpp`、`src/network/NetworkManager.cpp`、`tests/unit/test_rmul2027_transport.cpp`、`tests/CMakeLists.txt`。

- [x] 增加生产GameData回归：血量400、弹量90后仅更新热量，血量与弹量保持；显式零仍更新为零。
- [x] 增加静态、模块、经济的缺失字段测试；键鼠释放序列化后七个字段均具有存在性。
- [x] 运行测试确认旧实现失败。
- [x] 为本阶段消费的官方字段添加optional，移除动态字段4的能量用途，模块字段11使用安装权限语义；更新各消息的增量处理和网络转换。
- [x] 同步生成Python协议并运行C++/Python相关测试。

## 任务二：控制发送与自定义回传

文件：`src/network/NetworkManager.*`、`src/ui/MainWindow.cpp`、`src/network/ControlSendPolicy.h`、相关测试及模拟器协议代码。

- [x] 测试60字节/61字节边界、75Hz预算、释放帧最终发送与断连清理。
- [x] 单调时钟调度发送，合并为最新状态；键鼠和自定义控制各自遵守预算，发送失败不声明成功。
- [x] 自定义下行提供原始数据信号，视频与旧状态仅在显式选择对应队内格式时解析；默认不按长度猜测。
- [x] 通过隔离本机Broker验证真实MQTT收发，不使用正在运行的3333端口。

## 任务三：地图协议与迁移边界

文件：`src/network/proto/robomaster.proto`、`src/network/NetworkManager.*`、`src/ui/MainWindow.cpp`、`src/core/MapCoordinateMapper.*`、模拟器与测试。

- [x] 增加2027上行MapClickCmd、下行MapClickInfo和固定7字节目标列表测试。
- [x] 改正Topic与接收路由，保证收到标记只更新显示。
- [x] 清理操作页28×15坐标假设；在坐标原点与图像对应未经设备验证时，地图显示和标记保持本地用途，并明确界面状态。
- [x] 删除官方订阅中的已取消消息；保留历史内部类型兼容时明确其边界。

## 验收与记录

- [x] Release构建及完整CTest；主窗口集成测试使用隔离连接配置。
- [x] Python协议、数据、设置、结算、比赛引擎测试；隔离MQTT链路验证。
- [x] 更新协议manifest、阶段文档和AGENTS，记录实际验证与设备限制。
- [x] 检查差异与工作区，提交成果并创建完成标签。
