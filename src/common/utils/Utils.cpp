#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// RadioMesh includes
#include <RadioMeshVersion.h>
#include <common/inc/Definitions.h>
#include <common/inc/Logger.h>
#include <common/utils/Utils.h>

namespace RadioMeshUtils
{

std::string getVersion()
{
    return std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." +
           std::to_string(VERSION_PATCH) + "-" + std::to_string(VERSION_EXTRA);
}

uint8_t simpleRNG(uint16_t size)
{
    uint8_t val;
#if defined(ESP32)
    uint8_t analogPin = A0;
#else
    uint8_t analogPin = GPIO0;
#endif
    val = 0;
    while (size) {
        for (unsigned i = 0; i < 8; ++i) {
            int init = analogRead(analogPin);
            // Instead of waiting for change, just sample twice
            delayMicroseconds(1); // Tiny delay between reads
            int second = analogRead(analogPin);

            // Use difference between readings
            int diff = abs(second - init);
            val = (val << 1) | (diff & 0x01);
        }
        val++;
        --size;
    }

    // If we got 0, just use millis as fallback
    if (val == 0) {
        val = (millis() & 0xFF) + 1;
    }

    return val;
}

std::string createUuid(int length)
{
    std::string msg = "";
    int i;

    for (i = 0; i < length; i++) {
        byte randomValue = random(36);
        if (randomValue < 26) {
            msg = msg + char(randomValue + 'a');
        } else {
            msg = msg + char((randomValue - 26) + '0');
        }
    }
    return msg;
}

std::string convertToHex(const byte* data, int size)
{
    // Note: This function is not thread safe
    std::string buf = ""; // static to avoid memory leak
    buf.clear();
    buf.reserve(size * 2); // 2 digit hex
    const char* hex = "0123456789ABCDEF";
    for (int i = 0; i < size; i++) {
        byte val = data[i];
        buf += hex[(val >> 4) & 0x0F];
        buf += hex[val & 0x0F];
    }
    return buf;
}

uint32_t toUint32(const byte* data)
{
    uint32_t value = 0;

    value |= data[0] << 24;
    value |= data[1] << 16;
    value |= data[2] << 8;
    value |= data[3];
    return value;
}

std::string toUpperCase(std::string str)
{
    std::string upper = "";
    for (int i = 0; i < str.length(); i++) {
        upper += toupper(str[i]);
    }
    return upper;
}

std::string wifiSignalToString(SignalIndicator signal)
{
    switch (signal) {
    case SignalIndicator::NO_SIGNAL:
        return "no signal";
    case SignalIndicator::WEAK:
        return "[|   ]";
    case SignalIndicator::FAIR:
        return "[||  ]";
    case SignalIndicator::GOOD:
        return "[||| ]";
    case SignalIndicator::EXCELLENT:
        return "[||||]";
    default:
        return "?";
    }
}

bool isBroadcastAddress(const std::array<byte, RM_ID_LENGTH>& address)
{
    if (address == BROADCAST_ADDR) {
        return true;
    }
    return false;
}

bool areDeviceIdsEqual(const std::array<byte, RM_ID_LENGTH>& id1,
                       const std::array<byte, RM_ID_LENGTH>& id2)
{
    return id1 == id2;
}

uint32_t deviceIdToUint32(const std::array<byte, RM_ID_LENGTH>& id)
{
    uint32_t value = 0;
    value |= id[0] << 24;
    value |= id[1] << 16;
    value |= id[2] << 8;
    value |= id[3];
    return value;
}

std::array<byte, RM_ID_LENGTH> uint32ToDeviceId(uint32_t value)
{
    return {static_cast<byte>((value >> 24) & 0xFF), static_cast<byte>((value >> 16) & 0xFF),
            static_cast<byte>((value >> 8) & 0xFF), static_cast<byte>(value & 0xFF)};
}

std::string toString(const std::vector<byte>& vec, DataFormat format)
{
    if (vec.empty()) {
        return "<<Empty>>";
    }

    std::string result;
    uint8_t value; // Use for consistent byte handling
    char hex[8];   // Keep larger buffer for safety

    switch (format) {
    case DataFormat::DECIMAL:
        for (const auto& b : vec) {
            if (!result.empty())
                result += " ";
            value = static_cast<uint8_t>(b);
            result += std::to_string(value);
        }
        break;

    case DataFormat::HEXD:
        for (const auto& b : vec) {
            value = static_cast<uint8_t>(b);
            snprintf(hex, sizeof(hex), "%02X", value);
            result += hex;
        }
        break;

    case DataFormat::HEXD_SPACED:
        for (const auto& b : vec) {
            if (!result.empty())
                result += " ";
            value = static_cast<uint8_t>(b);
            snprintf(hex, sizeof(hex), "%02X", value);
            result += hex;
        }
        break;

    case DataFormat::ASCII:
        for (const auto& b : vec) {
            value = static_cast<uint8_t>(b);
            result += std::isprint(value) ? static_cast<char>(value) : '.';
        }
        break;
    }

    return result;
}

} // namespace RadioMeshUtils
