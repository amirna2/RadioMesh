#include <unity.h>

#include <common/utils/RadioMeshCrc32.h>
#include <core/protocol/inc/packet/Packet.h>

void setUp(void) {}
void tearDown(void) {}

void test_env_smoke(void)
{
    TEST_ASSERT_EQUAL(1, 1);
}

void test_crc32_is_deterministic(void)
{
    const uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    RadioMeshUtils::CRC32 a;
    a.update(data, sizeof(data));
    RadioMeshUtils::CRC32 b;
    b.update(data, sizeof(data));
    TEST_ASSERT_EQUAL_HEX32(a.finalize(), b.finalize());
}

void test_crc32_detects_single_bit_change(void)
{
    const uint8_t data1[] = {0xDE, 0xAD, 0xBE, 0xEF};
    const uint8_t data2[] = {0xDE, 0xAD, 0xBE, 0xEE};
    RadioMeshUtils::CRC32 a;
    a.update(data1, sizeof(data1));
    RadioMeshUtils::CRC32 b;
    b.update(data2, sizeof(data2));
    TEST_ASSERT_NOT_EQUAL(a.finalize(), b.finalize());
}

void test_packet_header_and_payload_roundtrip(void)
{
    RadioMeshPacket pkt;
    pkt.protocolVersion = RM_PROTOCOL_VERSION;
    pkt.sourceDevId = {0x01, 0x02, 0x03, 0x04};
    pkt.destDevId = {0x05, 0x06, 0x07, 0x08};
    pkt.packetId = {0xAA, 0xBB, 0xCC, 0xDD};
    pkt.topic = 0x10;
    pkt.deviceType = 0x02;
    pkt.hopCount = 3;
    pkt.packetCrc = 0x12345678;
    pkt.fcounter = 42;
    pkt.lastHopId = {0x11, 0x22, 0x33, 0x44};
    pkt.nextHopId = {0x55, 0x66, 0x77, 0x88};
    pkt.packetData = {0xDE, 0xAD, 0xBE, 0xEF};

    std::vector<byte> buf = pkt.toByteBuffer();
    TEST_ASSERT_EQUAL_UINT32(HEADER_LENGTH + 4, buf.size());

    RadioMeshPacket parsed(buf);
    TEST_ASSERT_EQUAL(pkt.protocolVersion, parsed.protocolVersion);
    TEST_ASSERT_EQUAL(pkt.topic, parsed.topic);
    TEST_ASSERT_EQUAL(pkt.deviceType, parsed.deviceType);
    TEST_ASSERT_EQUAL(pkt.hopCount, parsed.hopCount);
    TEST_ASSERT_EQUAL_HEX32(pkt.packetCrc, parsed.packetCrc);
    TEST_ASSERT_EQUAL_UINT32(pkt.fcounter, parsed.fcounter);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pkt.sourceDevId.data(), parsed.sourceDevId.data(), DEV_ID_LENGTH);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pkt.destDevId.data(), parsed.destDevId.data(), DEV_ID_LENGTH);
    TEST_ASSERT_EQUAL_UINT32(pkt.packetData.size(), parsed.packetData.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pkt.packetData.data(), parsed.packetData.data(),
                                  pkt.packetData.size());
}

int main(int, char**)
{
    UNITY_BEGIN();
    RUN_TEST(test_env_smoke);
    RUN_TEST(test_crc32_is_deterministic);
    RUN_TEST(test_crc32_detects_single_bit_change);
    RUN_TEST(test_packet_header_and_payload_roundtrip);
    return UNITY_END();
}
