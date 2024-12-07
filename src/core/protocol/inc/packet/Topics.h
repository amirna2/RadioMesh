#pragma once

#include <common/inc/Options.h>

namespace radiomesh {
/**
 * @enum Topic
 * @brief Protocol control topics used by the RadioMesh protocol
 *
 * Topics 0x00-0x0F are reserved for protocol control
 * Application topics should start from 0x10
 */
enum Topic : uint8_t {
    UNUSED = 0x00,
    PING = 0x01,
    PONG = 0x02,
    ACK = 0x03,
    CMD = 0x04,
    BYE = 0x05,
    INCLUDE_REQUEST = 0x06,
    INCLUDE_RESPONSE = 0x07,
    INCLUDE_OPEN = 0x08,
    INCLUDE_CONFIRM = 0x09,
    MAX_RESERVED = 0x0F
};

// Helper to check if topic is a protocol topic
constexpr bool isProtocolTopic(uint8_t topic) {
    return topic <= Topic::MAX_RESERVED;
}
} // namespace radiomesh

