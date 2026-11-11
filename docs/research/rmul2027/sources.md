# 来源与检索记录

核查日期：2026-11-11。用户目标：2027 RMUL 3V3，复旦开源框架为基础。优先级：正式中文规则/协议 → 经核验的实际设备行为与官方澄清 → 开源实现与模拟器。开源代码、模拟器自洽均不能代替官方兼容性证据。

## 官方资料

本轮通过浏览器实际打开官方动态页面，核对了中文版版本列表；网页检索工具无法完整读取时没有将其当作文件不存在。

| 资料 | 官方入口 | 本地版本及页数 | 本次重点 |
| --- | --- | --- | --- |
| 通信协议 | [RMU 2027](https://bbs.robomaster.com/wiki/20204847/1969151) | V1.0.0，2026-10-29，85 PDF 页 | 印刷 p7、9–11、38、46–74；32 Topic、图传、自定义上下行 |
| RMUL 比赛规则 | [RMUL 2027](https://bbs.robomaster.com/wiki/20204847/1969150) | V1.0.0，2026-10-15，68 PDF 页 | 印刷 p16–23、35–38；3V3 阵容、终端、场地、控制度、重装 |
| 机器人制作规范 | [RMUL 2027 同页](https://bbs.robomaster.com/wiki/20204847/1969150) | V1.0.0，2026-10-15，91 PDF 页 | 印刷 p16–18；UI 与终端硬件要求 |
| RMUL 参赛手册 | [RMUL 2027 同页](https://bbs.robomaster.com/wiki/20204847/1969150) | V1.0.0，2026-10-15，28 PDF 页 | 赛事背景与资料归档；不作为通信字段来源 |

页码约定：以上 PDF 的 **PDF 第 N 页 = 印刷 p(N−1)**。检索库的 `<!-- page N -->` 表示 PDF 页序，不是印刷页码。

### 本地原文件与 SHA-256

| ID | 原文件 | SHA-256 |
| --- | --- | --- |
| protocol-2027 | [通信协议](<D:/MyTrain/robomaster/文档/通信协议/RoboMaster 2027 机甲大师高校系列赛通信协议 V1.0.0（20261029）.pdf>) | `109a5e2a82afd3f253a5ac15aef7bb5e0aa354c6b41e3a6ceca7fed889e619e1` |
| rmul-rules-2027 | [RMUL 规则](<D:/MyTrain/robomaster/文档/比赛规则/2027赛季/高校联盟赛/RoboMaster 2027 机甲大师高校联盟赛比赛规则手册 V1.0.0（20261015）.pdf>) | `a8fd7b7f6ea5212b9e4dfa590b478827992598d91912ed671653c8bdd9727fd6` |
| robot-spec-2027 | [制作规范](<D:/MyTrain/robomaster/文档/比赛规则/2027赛季/通用规范/RoboMaster 2027 机甲大师高校系列赛机器人制作规范手册V1.0.0（20261015）.pdf>) | `3287fe5f521f8801ce69632032ea17a9c8d8a099091558f58618cd7721154a3c` |
| rmul-participant-2027 | [参赛手册](<D:/MyTrain/robomaster/文档/比赛规则/2027赛季/高校联盟赛/RoboMaster 2027 机甲大师高校联盟赛参赛手册V1.0.0（20261015）.pdf>) | `969fb865b7dca28360325d198fd47a7158cb7c8cfe3e0fd6d51bc556b67b2950` |

官方通信协议下载链接（本轮从网页可见链接核对）：

[2027 中文通信协议 PDF](https://bbs-web-static.robomaster.com/3db568c5796b494190475a694f001e841793263540152/RoboMaster%202027%20%E6%9C%BA%E7%94%B2%E5%A4%A7%E5%B8%88%E9%AB%98%E6%A0%A1%E7%B3%BB%E5%88%97%E8%B5%9B%E9%80%9A%E4%BF%A1%E5%8D%8F%E8%AE%AE%20V1.0.0%EF%BC%8820261029%EF%BC%89.pdf)。

本轮核对的是网页可见的中文基线；未重新下载全部远端 PDF 逐字节比对。官方页面英文协议一行的发布日期与文件日期存在不一致，未采用其日期推断中文版更新。

## 开源与技术文档

| 来源 | 本轮确认 | 对本项目的用途与限制 |
| --- | --- | --- |
| [ClearWei/RM26_Client](https://github.com/ClearWei/RM26_Client)；[作者开源文章](https://bbs.robomaster.com/article/1943329) | 本地 `main` 快照 `e291f2f65f5a5924509e4e2fe1b148ff9d4b24af`；MIT；2026 V2.0.0 协议清单仍为 in-progress | 主体框架，保留原版权及许可；本轮主要依据本地源代码，而非文章介绍 |
| [JFCY2718/rmuc-custom-client-ui](https://github.com/JFCY2718/rmuc-custom-client-ui) | README 描述 2026 React/Tauri 客户端；本机通信验证与真实裁判系统验收分开；未宣称完成真实比赛验收 | 参考缺失/过期字段展示、诊断和联调思路；不替换 Qt 主框架，不当作 2027 适配器 |
| [dkjsiogu/rm-client](https://github.com/dkjsiogu/rm-client) | 2026 Web/MQTT/视频桥接方向；旧版本协议 | 仅作不同实现路线参考；本轮未构建或逐项审计 |
| [JNU-SHARK/SharkDataSever](https://github.com/JNU-SHARK/SharkDataSever) | 2026 自定义客户端模拟数据项目 | 参考协议观测与模拟数据组织；不能直接证明 2027 可用 |
| [RM2027 自定义终端通信接口 开源](https://bbs.robomaster.com/article/1972652?source=1) | 标题和参考文献可读；网页与浏览器均未显示接口正文或独立下载地址 | 记录为后续线索，**未据标题认定已找到可复用的 2027 通信实现** |
| [minimal_hik_gimbal_bridge](https://github.com/dkjsiogu/minimal_hik_gimbal_bridge) | 2026 工业相机与自定义字节链路参考项目 | 第二视频通道参考；不绕过 300B/50Hz 预算，不当作主图传 |
| [Protobuf 字段存在性官方指南](https://protobuf.dev/programming-guides/field_presence/) | 显式/隐式字段存在性的技术依据 | 理解 optional 与零值；实际 RM 字段以当季协议为准 |

搜索范围包含 2027 通信协议、RMUL 3V3、自定义终端、复旦 EGA 及其他开源终端；不将搜索摘要或第三方规则汇编作为官方规则依据。

## 官方实时答疑访问核验（2026-11-11 补充）

已从[组委会渠道与答疑规范公告](https://www.robomaster.com/zh-CN/resource/pages/announcement/1208/)确认2027官方入口，并实际通过浏览器读取[RMU 2027规则答疑公示面板](https://qingflow.com/appView/g79clgn1kk02/shareView/g79clh9dkk02)。页面当前显示共437条，按编号降序、50条/页，共9页；可读取编号、赛事类型、问题、官方反馈及附件入口。普通网页文本工具返回空正文，但浏览器加载后可读取表格，因此不能把文本抓取为空认定为无访问权限。

本次只验证访问并抽看首页，未逐条审计437条内容，未查看全部附件，也未核实所有条目的更新时间。实际看到RMUL与RMUC条目混排，后续必须保留赛事类别，不能直接跨赛事套用结论。第三方[BirdieBot整理页](https://birdiebot.org/rules/)显示最近同步2026-11-09，可作检索线索；最终仍回查官方面板。此记录是访问快照，不是持续自动监测或更新完整性承诺。

## 本地延续资料

- [既有终端总调研](<D:/MyTrain/robomaster/docs/rm-custom-client-2027.md>)：通用通信链路和制作条件背景。本次补充 RMUL 专项，覆盖旧资料中不适用于 3V3 的 RMUC 假设。
- [既有机械臂链路计划](<D:/MyTrain/robomaster/docs/superpowers/plans/2026-11-07-custom-controller-vt-link.md>)：其中 28B 六轴载荷和 MCU 直接网关属于原先方案，不代表已接通。采用桌面 Qt 作为网关仍需讨论确认。
- `D:/MyTrain/robomaster/electronic_control/engineer_x/modules/referee/referee_protocol.h`：发现 `0x0302`、30B 的旧定义，未找到 `0x0311`；本轮未改固件、未烧录。不能据终端侧发布成功判定机器人能接收。

## 提取与质量记录

使用 `pdf-to-chunks` 技能生成协议 50 个章节块、RMUL 规则 95 个章节块；另用 PyMuPDF 为四份 PDF 建立逐页全文与 SHA-256 清单。低内容页：规则 PDF 1/44/68，协议 PDF 1/85；其中封面/图片页不能当作空白资料。关键表格及异常协议示例另渲染图像核对。

原有分块曾遗漏 `CustomControl` 等表格。新章节块也不能保证表格完整，因此查参数顺序为：章节定位 → 逐页全文 → 原 PDF/渲染页核对。已视觉核对规则 PDF17 的阵容、PDF24 的场地图及协议 PDF69 的异常雷达定义。

字段比较使用从 PDF 消息块提取的编号/类型/名称/presence，与当前仓库 `.proto` 对照；没有把 PDF 示例拼成新的生产 schema。官方示例中的错误原样记为待澄清项。
