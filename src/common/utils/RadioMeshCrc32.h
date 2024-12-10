#pragma once

#include <Arduino.h>

namespace RadioMeshUtils
{
class CRC32
{
public:
   CRC32()
   {
      reset();
   }

   inline void reset()
   {
      crc = ~0L;
   }

   // Process a single byte with reflection
   inline void update(uint8_t value)
   {
      value = reflectByte(value);
      crc ^= static_cast<uint32_t>(value);
      for (int i = 0; i < 8; i++) {
         if (crc & 1) {
            crc = (crc >> 1) ^ polynomial;
         } else {
            crc >>= 1;
         }
      }
   }

   // Update with uint16 and uint32 values as before
   inline void update(uint16_t value)
   {
      update(static_cast<uint8_t>(value & 0xFF));
      update(static_cast<uint8_t>((value >> 8) & 0xFF));
   }

   inline void update(uint32_t value)
   {
      update(static_cast<uint16_t>(value & 0xFFFF));
      update(static_cast<uint16_t>((value >> 16) & 0xFFFF));
   }

   // Update with a byte array
   inline void update(const uint8_t* data, size_t length)
   {
      for (size_t i = 0; i < length; i++) {
         update(data[i]);
      }
   }

   // Finalize and return the reflected CRC
   inline uint32_t finalize()
   {
      return reflect(crc) ^ ~0L;
   }

private:
   uint32_t crc;
   static constexpr uint32_t polynomial = 0xEDB88320;

   // Reflects a byte, flipping the bit order
   inline uint8_t reflectByte(uint8_t b)
   {
      uint8_t reflection = 0;
      for (uint32_t i = 0; i < 8; i++) {
         if (b & (1U << i)) {
            reflection |= (1 << (7 - i));
         }
      }
      return reflection;
   }

   // Reflects a 32-bit value, flipping the bit order
   inline uint32_t reflect(uint32_t data)
   {
      uint32_t reflection = 0;
      for (uint32_t i = 0; i < 32; i++) {
         if (data & (1U << i)) {
            reflection |= (1U << (31 - i));
         }
      }
      return reflection;
   }
};
} // namespace RadioMeshUtils
