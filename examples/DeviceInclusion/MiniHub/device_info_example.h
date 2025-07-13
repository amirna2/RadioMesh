#pragma once

#include <Arduino.h>
#include <RadioMesh.h>
#include <array>
#include <string>
#include <vector>

#ifdef RM_NO_DISPLAY
#pragma message "Display is disabled"
#else
#define USE_DISPLAY
#endif

#ifdef RM_NO_WIFI
#pragma message "Wifi is disabled"
#else
#define USE_WIFI
#define USE_WIFI_AP
#endif

// Default device configuration
// DeviceID for testing only. Actual device ID could be stored in EEPROM or use part of ESP32 chipID
const std::array<byte, RM_ID_LENGTH> DEVICE_ID = {0x77, 0x77, 0x77, 0x77};

// Provided as example
const std::string DEVICE_NAME = "MiniHub";
const std::string WIFI_AP_SSID = "MiniHubAP";
const std::string WIFI_AP_PASSWORD = "firefly2517";
const std::string WIFI_AP_IP_ADDRESS = "192.168.20.1";

// Change this to your WIFI access. This could also be set during runtime via a web interface
const std::string WIFI_SSID = "your_wifi_ssid";
const std::string WIFI_PASSWORD = "your_wifi_password";

#ifdef USE_HELTEC_WIFI_LORA_32_V3
LoraRadioParams radioParams = LoraRadioPresets::HELTEC_WIFI_LORA_32_V3;
#endif

#ifdef USE_XIAO_ESP32S3_WIO_SX1262
LoraRadioParams radioParams = LoraRadioPresets::XIAO_ESP32S3_WIO_SX1262;
#endif

#ifdef USE_CUBECELL
LoraRadioParams radioParams = LoraRadioPresets::HELTEC_CUBECELL;
#endif

#ifdef USE_DISPLAY
OledDisplayParams displayParams = OledDisplayParams(SCL_OLED, SDA_OLED, RST_OLED, RM_FONT_SMALL);
#endif

#ifdef USE_WIFI
WifiParams wifiParams = {WIFI_SSID, WIFI_PASSWORD};
#endif

#ifdef USE_WIFI_AP
WifiAccessPointParams apParams = {WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_IP_ADDRESS};
#endif
