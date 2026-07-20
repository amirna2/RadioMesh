#pragma once

#include <cstddef>
#include <cstdint>

class IRadio;
class IByteStorage;

/**
 * @namespace RadioMeshPlatform
 * @brief Platform services contract for the RadioMesh core.
 *
 * The portable core (common/, core/, framework/) contains no platform code:
 * every kernel service it needs is declared here and provided by exactly one
 * platform port selected at build time:
 *  - Arduino: src/hardware/src/platform/ArduinoPlatform.cpp
 *  - Zephyr:  ports/zephyr/hal/src/platform/ZephyrPlatform.cpp
 *  - Host:    common/inc/platform/HostPlatform.h (inline reference
 *             implementation for native builds and unit tests)
 */
namespace RadioMeshPlatform
{
/**
 * @brief Get the number of milliseconds elapsed since boot (monotonic).
 *
 * @returns Milliseconds since boot.
 */
uint32_t millis();

/**
 * @brief Block the calling context for the given duration.
 *
 * @param ms Duration in milliseconds.
 */
void delay(uint32_t ms);

/**
 * @brief Generate a uniform random number in [0, max).
 *
 * @param max Exclusive upper bound. Returns 0 when max is 0.
 * @returns A random number in [0, max).
 */
uint32_t random(uint32_t max);

/**
 * @brief Fill a buffer with the best entropy the platform offers.
 *
 * @param buf Buffer to fill.
 * @param len Number of bytes to generate.
 */
void randomBytes(uint8_t* buf, size_t len);

/**
 * @brief Get a per-chip unique identifier.
 *
 * Used to derive the device key seed; must be stable across boots and
 * unique per physical device.
 *
 * @returns A 64-bit chip identifier.
 */
uint64_t chipId();

/**
 * @brief Bring up the logging console.
 *
 * No-op on platforms whose console needs no initialization.
 */
void consoleInit();

/**
 * @brief Write raw log output (the sink behind the Logger macros).
 *
 * @param data Bytes to write.
 * @param len Number of bytes.
 * @returns The number of bytes written.
 */
size_t logWrite(const uint8_t* data, size_t len);

/**
 * @brief Get the platform's LoRa radio implementation.
 *
 * Composition root for the radio: each platform returns its IRadio
 * implementation (Arduino: LoraRadio, Zephyr: ZephyrLoraRadio).
 *
 * @returns Pointer to the platform radio, or nullptr if unavailable.
 */
IRadio* loraRadio();

/**
 * @brief Get the platform's byte storage implementation.
 *
 * Composition root for storage: each platform returns its IByteStorage
 * implementation (Arduino: EEPROMStorage, Zephyr: ZephyrNVSStorage).
 *
 * @returns Pointer to the platform storage, or nullptr if unavailable.
 */
IByteStorage* byteStorage();
} // namespace RadioMeshPlatform

#if !defined(ARDUINO) && !defined(__ZEPHYR__)
// Native/host builds (unit tests, future Linux port) use the inline reference
// implementation; embedded platforms link their port's implementation file.
#include <common/inc/platform/HostPlatform.h>
#endif
