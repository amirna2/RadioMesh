/**
 * WARNING: This file is given as an example.
 * You should create your own "device_info.h" file
 *
 * The encryption keys and configuration variables are ONLY for demonstration purposes.
**/

#pragma once

#include <Arduino.h>
#include <vector>
#include <string>
#include <array>
#include <RadioMesh.h>

std::vector<byte> key = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                        };

std::vector<byte> iv = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};



// Default device configuration - change these values to match your network configuration
const std::array<byte, RM_ID_LENGTH> DEVICE_ID = {0x99, 0x99, 0x99, 0x99};
const std::string DEVICE_NAME = "Hub";
const std::string WIFI_AP_SSID = "HubPortalAP";
const std::string WIFI_AP_PASSWORD = "firefly2517";
const std::string WIFI_AP_IP_ADDRESS = "192.168.20.1";

const std::string WIFI_SSID = "ssid_example";
const std::string WIFI_PASSWORD = "wifi_password_example";

