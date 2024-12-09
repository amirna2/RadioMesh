<p align="center">
  <a href="https://www.apache.org/licenses/LICENSE-2.0"><img src="https://img.shields.io/badge/License-Apache2-0AC0E9.svg" alt="License"></a>
  <a href="https://github.com/amirna2/RadioMesh/wiki"><img src="https://img.shields.io/badge/Read-Wiki-50dda0" alt="Wiki"></a>
  <a href="https://amirna2.github.io/RadioMesh"><img src="https://img.shields.io/badge/API-Doc-50dda0" alt="Api Doc"></a>
  <img src="https://img.shields.io/github/v/release/amirna2/RadioMesh?include_prereleases&label=Release&color=10aaff" alt="Release">
  <img src="https://github.com/amirna2/RadioMesh/actions/workflows/main_ci.yml/badge.svg" alt="Build Status">
</p>

# RadioMesh
A library for building wide area mesh networks of embedded devices using long range radios.
Perfect for sensor networks, IoT applications, and remote control/monitoring systems.

RadioMesh combines an adaptive wireless mesh protocol design with an easy to use and flexible device development kit.
It leverages [LoRa](https://www.semtech.com/lora) radios and the Arduino framework.

## Core Components

### Wireless Mesh Protocol
At its core, RadioMesh implements a mesh network protocol aimed at self-organizing, low-power, long-range devices. It's focused on rapid and secure deployment of small to medium-sized networks without extensive configuration or management. key features (current and planned) include:

- Datagram-based messaging
- Two-way communication
- Passive route learning
- Multi-hop message delivery
- Adaptive role-based network topology
- Power-efficient operation

### Device Framework
Additionally, RadioMesh provides an developer friendly framework for building mesh-capable devices:
- Hardware abstraction interfaces (Display, Radio, WiFi)
- Modular component architecture
- Security and encryption
- Easy-to-use builder pattern for device configuration

### Currently Supported Boards
- Heltec Wifi Lora 32 v3
- Seeed Studio Xiao ESP32 WIO-SX1262
- Heltec CubeCell ASR650X series, e.g: CubeCell Board v2, CubeCell Board Plus,...

### Steps To Get Started
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

    
## Contributing
RadioMesh welcomes contributions! Whether you're interested in adding new features, fixing bugs, improving documentation, or sharing example applications, check out the [Contributing Guide](CONTRIBUTING.md) to get started.
