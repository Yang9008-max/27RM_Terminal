# 在 Ubuntu 上运行 RMUL 2027 自定义终端

本文使用 Ubuntu 24.04 LTS。打开 Linux 电脑的终端，执行 `uname -m` 查看处理器架构。Intel 和 AMD 电脑通常显示 `x86_64`，ARM 电脑通常显示 `aarch64`。两种架构各自编译，生成的程序与其架构对应。

## 安装依赖

Qt 用于界面、键鼠和声音，Paho MQTT 和 Protobuf 用于收发比赛数据，FFmpeg 用于解码图传。首次安装需要网络。以下命令使用 Ubuntu 的软件包管理器。

```bash
sudo apt update
sudo apt install -y \
  fonts-noto-cjk libxcb-cursor0 qt6-wayland \
  qt6-qpa-plugins libqt6concurrent6t64 libqt6widgets6t64 \
  libqt6multimedia6 libqt6network6t64 libqt6serialport6 libqt6svg6 \
  libqt6quickwidgets6 libpaho-mqtt1.3 libprotobuf32t64 libabsl20220623t64 \
  libavcodec60 libavformat60 libavutil58 libswscale7 \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-libav \
  gstreamer1.0-pulseaudio \
  qml6-module-qtqml qml6-module-qtqml-models qml6-module-qtqml-workerscript \
  qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
  qml6-module-qtquick-templates qml6-module-qtquick-window
```

收到已编译的压缩包时，可以直接跳到下面的启动步骤。需要本机从源码编译时，再安装这些工具。

```bash
sudo apt install -y \
  build-essential cmake ninja-build pkg-config libxkbcommon-dev \
  qt6-base-dev qt6-declarative-dev qt6-multimedia-dev qt6-serialport-dev qt6-svg-dev \
  libabsl-dev libpaho-mqtt-dev libprotobuf-dev protobuf-compiler \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
```

## 编译和启动

从源码目录执行以下命令。最后一条在个人目录生成独立的运行目录。旧版可执行文件名 `RoboMasterClient2025` 沿用了复旦框架的命名。

```bash
cmake --preset linux
cmake --build --preset linux --parallel 4
cmake --install build/linux --prefix "$HOME/rmul-terminal"
```

从压缩包部署时，在 Linux 电脑上解压到个人目录，进入包含可执行文件的那一层。按下列方式启动，第一次会从模板生成本机配置。更新程序时可保留自己的 `config.json`。

```bash
cd "$HOME/rmul-terminal"
if [ ! -e config.json ]; then cp config.example.json config.json; fi
./RoboMasterClient2025
```

模板中的 MQTT 地址为 `127.0.0.1:3333`，适合同一台电脑上的模拟器。独立模拟器和比赛现场使用各自的配置，临时联调可通过 `RM_MQTT_BROKER`、`RM_MQTT_PORT` 和 `RM_VIDEO_STREAM_URL` 覆盖。修改本机配置后重启客户端。[参数和配置文件说明](configuration.md)

启用可选的 AR 目标识别时，可在配置 CMake 时传入 `-DONNXRUNTIME_ROOT=/path/to/sdk`。使用与电脑架构对应的 Linux SDK，并将其共享库目录配置到系统加载路径。

## 验证

测试可在 Linux 桌面或无显示器环境运行。下面第二项用本地临时 Mosquitto 验证真实的 MQTT 收发，需要先安装 Ubuntu 的 `mosquitto` 和 `python3`。

```bash
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software RM_DISABLE_AUDIO=1 \
  ctest --preset linux --no-tests=error --timeout 120
python3 tools/verify_rmul2027_transport.py \
  --receiver build/linux/tests/test_rmul2027_transport
```

现场还需在最终使用的 Linux 电脑上插网线，确认本机有线地址、裁判系统 MQTT、UDP 主图传、音频输出和小机械臂实际采样链路。开发环境模拟器使用回环数据，现场网络参数按比赛要求单独配置。
