#pragma once
#include <Arduino.h>
#include <vector>
#include <string>
#include <array>
#include <Warp.h>

// WARNING: The encryption keys are ONLY for demonstration purposes.
// Please refer to the inclusion process documentation to understand how
// to securely include devices in the network.
std::vector<byte> key = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                        };

std::vector<byte> iv = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

// Default device configuration
const std::array<byte, WARP_ID_LENGTH> DEVICE_ID = {0x88, 0x88, 0x88, 0x88};

const std::string DEVICE_NAME = "WarpLeaf";