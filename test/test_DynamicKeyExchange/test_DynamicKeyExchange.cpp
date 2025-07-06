#include <RadioMesh.h>
#include <unity.h>

// Test constants - following test_Device.cpp pattern
std::array<byte, DEV_ID_LENGTH> id = {0x11, 0x11, 0x11, 0x11};
std::string deviceName = "testDevice";

// AES key and IV - following existing pattern
std::vector<byte> key = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x11, 0x22, 0x33,
                         0x44, 0x55, 0x66, 0x77, 0x88, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                         0x77, 0x88, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

std::vector<byte> iv = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

// New key for testing updates
std::vector<byte> newKey = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                            0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                            0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                            0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};

std::vector<byte> newIv = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
                           0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};

RadioMeshDevice createDevice()
{
    return RadioMeshDevice("testDevice", id, MeshDeviceType::STANDARD);
}

void test_updateSecurityParams_with_crypto(void)
{
    RadioMeshDevice device = createDevice();
    
    // Initialize crypto with initial network key
    SecurityParams initialParams;
    initialParams.method = SecurityMethod::AES;
    initialParams.key = key;  // Initial network key
    initialParams.iv = iv;
    
    int rc = device.initializeAesCrypto(initialParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
    
    // Update with new network key (e.g., during key rotation)
    SecurityParams newParams;
    newParams.method = SecurityMethod::AES;
    newParams.key = newKey;  // New network key
    newParams.iv = newIv;
    
    rc = device.updateSecurityParams(newParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_updateSecurityParams_without_crypto_creates_crypto(void)
{
    RadioMeshDevice device = createDevice();
    
    // Don't initialize crypto first - this tests dynamic crypto creation
    // Try to update security params without initializing crypto first
    // This should now succeed by creating AesCrypto on-demand
    SecurityParams newParams;
    newParams.method = SecurityMethod::AES;
    newParams.key = key;  // Network key received during inclusion
    newParams.iv = iv;
    
    int rc = device.updateSecurityParams(newParams);
    // This should now succeed because updateSecurityParams creates AesCrypto on-demand
    // when crypto is null, enabling dynamic security initialization for standard devices
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_updateSecurityParams_with_crypto);
    RUN_TEST(test_updateSecurityParams_without_crypto_creates_crypto);
    UNITY_END();
}

void loop()
{
    // Empty loop
}