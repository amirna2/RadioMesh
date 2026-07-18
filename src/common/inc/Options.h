#pragma once

#if ARDUINO >= 100
// Arduino build
#include "Arduino.h"
#define RM_ARDUINO_BUILD
#else
// generic build
#include <cstdint>
#include <cstdio>
typedef uint8_t byte;
#define RM_GENERIC_BUILD
#endif

// enable verbose logging
// #define RM_LOG_DEBUG

// #define RM_LOG_INFO
