#pragma once

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

/**
 * Inline host reference implementation of the RadioMeshPlatform contract,
 * used by native builds (unit tests, future Linux port). Included only by
 * RadioMeshPlatform.h when the build is neither Arduino nor Zephyr; embedded
 * platforms provide their implementation in their own port file instead.
 */
namespace RadioMeshPlatform
{
inline uint32_t millis()
{
    using namespace std::chrono;
    static const steady_clock::time_point start = steady_clock::now();
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now() - start).count());
}

inline void delay(uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline uint32_t random(uint32_t max)
{
    // Mirrors the pre-port native behavior (std::rand based, non-crypto).
    return (max == 0) ? 0 : static_cast<uint32_t>(std::rand() % max);
}

inline void randomBytes(uint8_t* buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        buf[i] = static_cast<uint8_t>(std::rand() & 0xFF);
    }
}

inline uint64_t chipId()
{
    // Mirrors the pre-port non-ESP32 fallback seed.
    return 12345;
}

inline void consoleInit()
{
}

inline size_t logWrite(const uint8_t* data, size_t len)
{
    return fwrite(data, 1, len, stdout);
}

inline IRadio* loraRadio()
{
    return nullptr; // no host radio implementation yet (future ports/host)
}

inline IByteStorage* byteStorage()
{
    return nullptr; // no host storage implementation yet (future ports/host)
}
} // namespace RadioMeshPlatform
