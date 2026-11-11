# Windows 本地依赖与启动

项目目录为 `D:\MyTrain\projects\27RM_Terminal`。原生依赖安装在项目内的 `.deps/msys64`，使用 UCRT64 GCC；模拟器使用 `sim/.venv` 内的 Python 3.13。脚本仅设置当前进程的环境变量。

## 依赖范围

- GCC、CMake、Ninja、pkgconf、ccache。
- Qt 6 Core、Concurrent、Gui、Widgets、Network、Multimedia、SerialPort、Svg、Qml、Quick、QuickWidgets、QuickControls2、Test，以及开发接口可用的 HttpServer、WebSockets。
- Protobuf 和 protoc、Abseil、Paho MQTT C、FFmpeg、Mosquitto。
- ONNX Runtime Windows x64 CPU SDK 1.31.1。公开仓库未包含训练好的 RoboMaster 检测模型；安装运行库本身不会产生检测结果。
- `sim/pyproject.toml` 中全部 Python 依赖；已存在的 Node.js 用于模拟器前端测试。

Qt WebEngine 的历史可选功能未启用；当前 CMake 必需组件列表不包含它。这套 MinGW 环境用于客户端主界面、MQTT 和图传。

本机 Windows 的 System32 含同名 ONNX 占位 DLL，其搜索优先级高于 PATH，会使客户端启动返回 `0xc000007b`。构建脚本将官方 SDK 的 `onnxruntime.dll` 复制到客户端和测试可执行文件旁，确保加载正确版本；官方 `.lib` 可直接使用。`prepare-onnx.ps1` 只补齐上游使用的 `include/onnxruntime/` 头文件目录，上游业务代码未因此修改。

## 编译与运行

首次从 Git 检出本项目时，先恢复本机 CMake 预设；已有预设时保留现有配置：

```powershell
if (-not (Test-Path -LiteralPath CMakeUserPresets.json)) {
    Copy-Item tools/windows/CMakeUserPresets.example.json CMakeUserPresets.json
}
```

在项目根目录运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/windows/build.ps1
# 同时运行原有 C++ 测试
powershell -NoProfile -ExecutionPolicy Bypass -File tools/windows/build.ps1 -Test
```

双击根目录 `start-client.cmd` 启动客户端。上游可执行文件沿用 `RoboMasterClient2025.exe` 名称，位于 `build/windows-ucrt64/`。程序按上游行为全屏启动。

双击 `start-simulator.cmd` 启动模拟器和仅监听 `127.0.0.1:3333` 的 Mosquitto，然后打开 <http://127.0.0.1:8000/>。终止模拟器脚本时会关闭它自己启动的 Broker。若端口被其他程序占用，脚本会报告错误。

客户端构建时从 `config.example.json` 复制本机配置到构建目录。启动脚本以构建目录为工作目录。需要硬件通信时再调整该目录内的 `config.json`。

本次安装不改变上游赛事协议。2027 赛季 RMUL 适配需另行实施。

## 更新依赖

已安装的环境可以运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/windows/install-dependencies.ps1
```

MSYS2 必须执行完整更新，不能仅更新个别依赖。升级主要版本后应重新运行编译和测试。详细安装版本记录在本机 `.deps/native-packages-installed.txt`、`.deps/python-requirements-installed.txt`。

`CMakeUserPresets.json` 保存本机路径，已被上游 `.gitignore` 忽略；`.deps/` 通过 `.git/info/exclude` 排除。SDK、下载缓存和虚拟环境不应提交。

## 下载校验

- MSYS2 基础包 `msys2-base-x86_64-20260927.tar.xz`：SHA-256 `ea2f31a0b6ade63914ce441ffb022f0f6aa96982bfefa2326460a26d5fb01322`，与[官方发布页](https://github.com/msys2/msys2-installer/releases/tag/2026-09-27)一致。镜像源为 USTC，包管理器保持签名校验启用。
- ONNX Runtime `onnxruntime-win-x64-1.31.1.zip`：SHA-256 `5869e26bb25314375f2d5ef1877e3ea06b1543d6c710e6e4a1ec2f7177ab4b2c`，与[官方发布页](https://github.com/microsoft/onnxruntime/releases/tag/v1.31.1)一致。
- 官方说明：[MSYS2 更新](https://www.msys2.org/docs/updating/)、[基础包与校验](https://www.msys2.org/docs/installer/)。

## 模拟器检查

```powershell
sim/.venv/Scripts/python.exe -m pip check
cd sim
.venv/Scripts/python.exe -m unittest tests.test_match_simulation_engine tests.test_protocol_contract
cd ..
node --test sim/tests/test_map_geometry.js
```

原生 Windows Python 无法直接执行 `.sh`。上游 `test_run_sim_contract.py` 的 5 项测试依赖此行为，不能作为原生 Windows 入口的运行证明；本项目提供的 Windows 入口为 `tools/windows/start-simulator.ps1`。

## 本机验证结果（2026-11-11）

- GCC 16.2.0、CMake 4.4.5、Qt 6.12.0、C++ Protobuf 36.2、Paho C 1.3.15、FFmpeg 9.0.2、ONNX Runtime 1.31.1。
- Release 客户端和全部 C++ 测试构建成功，CTest **27/27 通过**。
- Python `pip check`、全部声明模块导入、Protobuf 生成代码 6.33.2 与运行时 6.33.6 兼容性检查通过。
- 比赛引擎和协议 Python 测试 **22/22 通过**；Node 几何测试 **3/3 通过**。完整 Python 发现为 28 项，其中 5 项直接执行 `.sh`，在原生 Windows 下失败，另 23 项通过。
- Windows 模拟器入口成功启动本机 Broker 和网页服务：HTTP 200，并实际订阅收到 `GameStatus` MQTT 数据。
- FFmpeg H.264、HEVC 编码检查通过；开发接口的 Qt HttpServer 配置查找通过。
- 客户端已启动为 `RM26 Custom Client`，存在可见窗口句柄且进程响应正常。
