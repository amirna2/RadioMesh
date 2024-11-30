#pragma once

#include <Arduino.h>
#include <vector>
#include <string>
#include <array>
#include <RadioMesh.h>


/**
 * This file contains the device configuration for the Router device.
 * Rename this file to device_info.h and change the values to match your network configuration.
 */

std::vector<byte> key = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                        };

std::vector<byte> iv = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};


const std::array<byte, RM_ID_LENGTH> DEVICE_ID = {0x77, 0x77, 0x77, 0x77};
const std::string DEVICE_NAME = "Router";