#include <unity.h>

#include <common/utils/RadioMeshCrc32.h>

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

int main(int, char**)
{
    UNITY_BEGIN();
    RUN_TEST(test_env_smoke);
    RUN_TEST(test_crc32_is_deterministic);
    RUN_TEST(test_crc32_detects_single_bit_change);
    return UNITY_END();
}
