<p align="center">
  <a href="https://tlo.mit.edu/understand-ip/exploring-mit-open-source-license-comprehensive-guide"><img src="https://img.shields.io/github/license/amirna2/RadioMesh
  " alt="License"></a>
  <a href="https://github.com/amirna2/RadioMesh/wiki"><img src="https://img.shields.io/badge/Read-Wiki-50dda0" alt="Wiki"></a>
  <a href="https://amirna2.github.io/RadioMesh"><img src="https://img.shields.io/badge/API-Doc-50dda0" alt="Api Doc"></a>
  <img src="https://img.shields.io/github/v/release/amirna2/RadioMesh?include_prereleases&label=Release&color=10aaff" alt="Release">
  <img src="https://github.com/amirna2/RadioMesh/actions/workflows/main_ci.yml/badge.svg" alt="Build Status">
</p>

# RadioMesh
A library for building wireless, low power mesh networks using long range radios.
Perfect for sensor networks, IoT devices, and remote control/monitoring systems.

RadioMesh combines an adaptive wireless mesh protocol design with an easy to use and flexible device development kit.
It leverages [LoRa](https://www.semtech.com/lora) radios, the Arduino framework and PlatformIO IDE for development.s

## Core Components

### Wireless Mesh Protocol
At its core, RadioMesh implements a mesh network protocol aimed at self-organizing, low-power, long-range devices. It's focused on rapid and secure deployment of small to medium-sized networks without extensive configuration or management. key features (current and planned) include:

- Datagram-based messaging
- Two-way communication
- Passive route learning
- Multi-hop message delivery
- Adaptive role-based network topology
- Device inclusion and exclusion
- Power-efficient operation

### Device Framework
Additionally, RadioMesh provides a developer friendly framework for building mesh-capable devices:
- Hardware abstraction interfaces (Display, Radio, WiFi)
- Modular component architecture
- Security and encryption
- Easy-to-use builder pattern for device configuration

### Currently Supported Boards
- [Heltec Wifi Lora 32 v3](https://heltec.org/project/wifi-lora-32-v3/) ESP32S3 with OLED display and SX1262 radio
- [Seeed Studio Xiao ESP32 WIO-SX1262](https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit/) Thumb size ESP32S3 board with SX1262 radio
- [Heltec CubeCell](https://heltec.org/project/htcc-ab01-v2/) ASR650X series with SX1262 radio , e.g: CubeCell Board v2, CubeCell Board Plus,...

## Steps To Get Started
- Install VSCode
- Install PlatformIO IDE
- Connect a board to your PC USB e.g [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/)
- Clone the project: `git clone https://github.com/amirna2/RadioMesh`
- From the project root
  - Navigate to the [SensorDevice](https://github.com/amirna2/RadioMesh/tree/main/examples/SensorDevice) example
  - Update the `device_config_example.h` as indicated.
  - Switch the `env.lib_deps` to use the local library: `${radio_mesh.local}`
  - Build and Deploy the example to the board: `../tools/builder.py build -t heltec_wifi_lora_32_V3 --clean --deploy`
- Open a serial terminal and check the output. The device will initialize and start sending a simple counter message periodically
- Repeat the build/deploy steps with the [HubDevice](https://github.com/amirna2/RadioMesh/tree/main/examples/HubDevice) example to setup a device-to-device communication
- Detailed instructions are in the [Wiki](https://github.com/amirna2/RadioMesh/wiki/Getting-Started)


## Contributing
RadioMesh welcomes contributions! Whether you're interested in adding new features, fixing bugs, improving documentation, or sharing example applications, check out the [Contributing Guide](CONTRIBUTING.md) to get started.
