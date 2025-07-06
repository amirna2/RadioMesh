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
    
    // Initialize crypto with initial params
    SecurityParams initialParams;
    initialParams.method = SecurityMethod::AES;
    initialParams.key = key;
    initialParams.iv = iv;
    
    int rc = device.initializeAesCrypto(initialParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
    
    // Update with new security params
    SecurityParams newParams;
    newParams.method = SecurityMethod::AES;
    newParams.key = newKey;
    newParams.iv = newIv;
    
    rc = device.updateSecurityParams(newParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_updateSecurityParams_without_crypto(void)
{
    RadioMeshDevice device = createDevice();
    
    // Don't initialize device - following pattern from test_Device.cpp
    // Try to update security params without initializing crypto
    SecurityParams newParams;
    newParams.method = SecurityMethod::AES;
    newParams.key = key;
    newParams.iv = iv;
    
    int rc = device.updateSecurityParams(newParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_STATE, rc);
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_updateSecurityParams_with_crypto);
    RUN_TEST(test_updateSecurityParams_without_crypto);
    UNITY_END();
}

void loop()
{
    // Empty loop
}