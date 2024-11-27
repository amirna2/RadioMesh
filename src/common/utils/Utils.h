#pragma once

#include <string>
#include <vector>
#include <array>
#include <common/inc/Definitions.h>

enum class DataFormat {
    DECIMAL,
    HEXD,
    HEXD_SPACED,
    ASCII
};

namespace RadioMeshUtils {

/**
 * @brief Get the version of the RadioMesh library.
 *
 * @returns A string representing the version of the RadioMesh library.
 */
std::string getVersion();

/**
 * @brief create a UUID
 * @param length The length of the UUID
 * @return std::string The UUID
*/
std::string createUuid(int length);

/**
 * @brief Convert a byte array to a hex string.
 *
 * @param data The byte array to convert.
 * @param size The size of the byte array.
 * @returns A string representing the byte array in hex.
 */
std::string convertToHex(const byte* data, int size);

/**
 * @brief Convert a byte array to a uint32_t.
 * @param data The byte array to convert.
 * @returns A uint32_t representing the byte array.
*/
uint32_t toUint32(const byte* data);

/**
 * @brief Convert a string to uppercase.
 *
 * @param str The string to convert.
 * @returns A string representing the input string in uppercase.
 */
std::string toUpperCase(std::string str);

/**
 * @brief Convert a vector of bytes to a string depending on the format.
 *
 * @param vec A vector to convert
 * @param format The format to use for the conversion
 * @returns A std::string representing the vector
 *
 */
std::string toString(const std::vector<byte>& vec, DataFormat format = DataFormat::DECIMAL);

/**
 * @brief Convert a signal indicator to a string.
 *
 * @param signal The signal indicator to convert.
 * @returns A string representing the signal indicator.
*/
std::string wifiSignalToString(SignalIndicator signal);


/**
 * @brief Generate a random number.
 *
 * @param size The size of the random number to generate.
 * @returns A random number.
 */
uint8_t simpleRNG(uint16_t size);

/**
 * @brief Check if an address is a broadcast address.
 * @param address The address to check.
 * @returns true if the address is a broadcast address, false otherwise.
 */
bool isBroadcastAddress(const std::array<byte, RM_ID_LENGTH>& address);

/**
 * @brief checks if device IDs are equal
 * @param id1 The first device ID
 * @param id2 The second device ID
 * @returns true if the device IDs are equal, false otherwise.
 */
bool areDeviceIdsEqual(const std::array<byte, RM_ID_LENGTH>& id1, const std::array<byte, RM_ID_LENGTH>& id2);

/**
 * @brief Convert a device ID to a uint32_t.
 * @param id The device ID to convert.
 * @returns A uint32_t representing the device ID.
 */
uint32_t deviceIdToUint32(const std::array<byte, RM_ID_LENGTH>& id);

/**
 * @brief Convert a uint32_t to a device ID.
 * @param value The uint32_t to convert.
 * @returns An array representing the uint32_t.
 */
std::array<byte, RM_ID_LENGTH> uint32ToDeviceId(uint32_t value);

/**
 * @brief Convert a number to a byte array.
 * @param number The number to convert.
 * @returns A vector<byte> representing the number.
 */
template <typename T>
std::vector<byte> numberToBytes(T number) {
    std::vector<byte> bytes(sizeof(T));
    for (size_t i = 0; i < sizeof(T); ++i) {
        bytes[i] = static_cast<byte>((number >> (8 * (sizeof(T) - 1 - i))) & 0xFF);
    }
    return bytes;
}

/**
 * @brief Convert a byte array to a number.
 * @param bytes The byte array to convert.
 * @returns The original number.
 */
template <typename T>
T bytesToNumber(const std::vector<byte>& bytes) {
    T number = 0;
    size_t limit = std::min(bytes.size(), sizeof(T));
    for (size_t i = 0; i < limit; ++i) {
        number = (number << 8) | bytes[i];
    }
    return number;
}

/**
 * @brief Generate random bytes.
 *
 * @param length
 * @return An array of random bytes
 */
template <std::size_t length>
std::array<byte, length> getRandomBytesArray()
{
  const char* digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  randomSeed(simpleRNG(4));
  std::array<byte, length> bytes;
  for (std::size_t i = 0; i < length; i++) {
    bytes[i] = digits[random(36)];
  }
  return bytes;
}
} // namespace RadioMeshUtils
