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

// Example key and IV for AES encryption
std::vector<byte> key = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x11, 0x22, 0x33,
                         0x44, 0x55, 0x66, 0x77, 0x88, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                         0x77, 0x88, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

std::vector<byte> iv = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

// Default device configuration
const std::array<byte, RM_ID_LENGTH> DEVICE_ID = {0x77, 0x77, 0x77, 0x77};
const std::string DEVICE_NAME = "PortalHub";
const std::string WIFI_AP_SSID = "HubPortalAP";
const std::string WIFI_AP_PASSWORD = "your_wifi_ap_password";
const std::string WIFI_AP_IP_ADDRESS = "192.168.20.1";

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
