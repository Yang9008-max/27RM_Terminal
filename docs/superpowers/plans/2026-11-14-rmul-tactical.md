# RMUL 3V3 Tactical Screen Implementation Plan

**Goal:** 将用户截图中的旧战术指挥页迁移为 2027 RMUL 3V3。

**Architecture:** 复用 GameData 已校验的比赛快照，新增只读 QML 投影，阵容固定为 1/3/7。
战术页保留切角边框、红蓝配色、血量/对比动画与独立相机，替换旧基地、经济、地图和分析组件。
场地仅显示依据规则的结构示意，不使用旧雷达坐标或虚构实时位置。

**Tech Stack:** Qt 6 / C++ / QML / QtTest，Windows UCRT64。

## Constraints

- 基线 `rmul2027-pre-06-tactical-20261114`；分支 `rmul2027/06-tactical`。
- 缺失字段显示等待，不把 0 当成未知；身份切换和新局清除按 GameData 现有逻辑执行。
- 不在客户端推断胜负、在线状态、金币复活或旧机制预告。
- 原官方安装包、PDF 和导出二进制素材留在外部资料目录。

## Tasks

- [x] 在 `tests/unit/test_tactical_command_page.cpp` 使用真实 GameData 添加红蓝视角、1/3/4/7 协议映射、缺失/零值、伤害与总血量验收；先运行确认旧页失败。
- [x] 在 `GameData.h/.cpp` 增加 `rmulTacticalData`，复用 `settlementSnapshot()`，仅完整三车血量才提供合计，保留 uint32 范围。
- [x] 重写 `TacticalCommandPage.qml` 的展示入口，增加独立的头部、三车阵容、统计对比与场地示意组件并登记 `qml.qrc`；禁止回退到旧 Mock。
- [x] 将 `TacticalLargeMapOverlay.qml` 接到同一场地示意，保留 M 键生命周期、布局切换与相机节流契约，更新几何测试为 12:8 比例。
- [x] 在 `MainWindow.cpp` 停止旧基地/前哨站预告和通过回血猜测金币复活的播报接入；保留显式开发测试入口。
- [x] Windows Release 构建与 CTest，真实窗口检查红蓝三车、归零、控制度、布局切换与大地图；记录限制并提交、打完成标签。

## Acceptance examples

`robot_health=[500,123,999,300,450,350,888,250]` 应显示红方合计 923、蓝方 1050，
4 号槽位不进展示或合计；控制度红 600/蓝 590。蓝方身份时 ally/enemy 数据转为对应颜色。
清空快照后为 `—`，收到 0 后为 `0`；不显示英雄、工程、空中、基地、前哨站、科技、侵垒或总经济。
