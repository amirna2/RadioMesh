#include <unity.h>
#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <core/protocol/inc/packet/Packet.h>
#include <core/protocol/inc/crypto/EncryptionService.h>
#include <core/protocol/inc/crypto/aes/AesCrypto.h>
#include <common/utils/Utils.h>
#include <cstring>
#include <EEPROM.h>

// ========================================
// SPECIFICATION COMPLIANCE TESTS
// ========================================

/**
 * @brief Inclusion Protocol Message Specifications
 * 
 * INCLUDE_OPEN:
 * - Encryption: None
 * - Key: N/A
 * - Payload: Hub public key (64 bytes)
 * - Expected Length: 64 bytes
 * 
 * INCLUDE_REQUEST:
 * - Encryption: Direct ECC
 * - Key: Hub public key
 * - Payload: Device ID (4) + Device public key (64) + Initial counter (4) = 72 bytes
 * - Expected Length: 72 bytes (zero overhead with direct ECC)
 * 
 * INCLUDE_RESPONSE:
 * - Encryption: Direct ECC
 * - Key: Device public key
 * - Payload: Hub public key (64) + Encrypted network key (32) + Nonce (4) = 100 bytes
 * - Expected Length: 100 bytes (zero overhead with direct ECC)
 * 
 * INCLUDE_CONFIRM:
 * - Encryption: AES
 * - Key: Shared network key
 * - Payload: Incremented nonce (4 bytes)
 * - Expected Length: 16 bytes (AES block size)
 * 
 * INCLUDE_SUCCESS:
 * - Encryption: AES
 * - Key: Shared network key
 * - Payload: Empty (0 bytes)
 * - Expected Length: 16 bytes (AES minimum block size)
 */

// Use ESP32's built-in ECC from TinyCrypt
extern "C" {
typedef struct uECC_Curve_t* uECC_Curve;
uECC_Curve uECC_secp256r1(void);
void uECC_set_rng(int (*rng_function)(uint8_t *dest, unsigned size));
int uECC_make_key(uint8_t *public_key, uint8_t *private_key, uECC_Curve curve);
}

// RNG function for micro-ecc
static int test_rng(uint8_t *dest, unsigned size) {
    while (size) {
        *dest++ = random(256);
        size--;
    }
    return 1;
}

// Generate valid ECC test keys
std::vector<byte> TEST_HUB_PRIVATE_KEY(32);
std::vector<byte> TEST_HUB_PUBLIC_KEY(64);  // Full 64-byte public key
std::vector<byte> TEST_DEVICE_PRIVATE_KEY(32);
std::vector<byte> TEST_DEVICE_PUBLIC_KEY(64);  // Full 64-byte public key

void generateTestKeys() {
    static bool keysGenerated = false;
    if (keysGenerated) return;
    
    uECC_set_rng(&test_rng);
    uECC_Curve curve = uECC_secp256r1();
    
    // Generate hub key pair
    uint8_t hubPrivKey[32];
    uint8_t hubPubKey[64];
    uECC_make_key(hubPubKey, hubPrivKey, curve);
    memcpy(TEST_HUB_PRIVATE_KEY.data(), hubPrivKey, 32);
    memcpy(TEST_HUB_PUBLIC_KEY.data(), hubPubKey, 64); // Full 64-byte key
    
    // Generate device key pair
    uint8_t devicePrivKey[32];
    uint8_t devicePubKey[64];
    uECC_make_key(devicePubKey, devicePrivKey, curve);
    memcpy(TEST_DEVICE_PRIVATE_KEY.data(), devicePrivKey, 32);
    memcpy(TEST_DEVICE_PUBLIC_KEY.data(), devicePubKey, 64); // Full 64-byte key
    
    keysGenerated = true;
}

const std::vector<byte> TEST_NETWORK_KEY = {
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
};

const std::array<byte, 4> TEST_DEVICE_ID = {0x11, 0x11, 0x11, 0x11};
const std::array<byte, 4> TEST_HUB_ID = {0x77, 0x77, 0x77, 0x77};

void setUp(void) {
    // Set up before each test - initialize Arduino and crypto system
    Serial.begin(115200);
    
    // Initialize EEPROM for crypto system
    EEPROM.begin(512);
    
    // Generate test keys
    generateTestKeys();
    
    delay(100);
}

void tearDown(void) {
    // Clean up after each test
    delay(10);
}

// ========================================
// ENCRYPTION SERVICE TESTS
// ========================================

void test_encryption_service_include_open() {
    EncryptionService service;
    
    // INCLUDE_OPEN should not be encrypted
    std::vector<byte> data = TEST_HUB_PUBLIC_KEY;
    std::vector<byte> result = service.encrypt(data, MessageTopic::INCLUDE_OPEN, 
                                              MeshDeviceType::HUB, 
                                              DeviceInclusionState::INCLUDED);
    
    // Should return original data unchanged
    TEST_ASSERT_EQUAL(64, result.size());
    TEST_ASSERT_EQUAL_MEMORY(data.data(), result.data(), 64);
}

void test_encryption_service_include_request() {
    EncryptionService service;
    service.setHubPublicKey(TEST_HUB_PUBLIC_KEY);
    service.setDeviceKeys(TEST_DEVICE_PRIVATE_KEY, TEST_DEVICE_PUBLIC_KEY);
    
    // Build INCLUDE_REQUEST payload: device ID + public key + counter
    std::vector<byte> payload;
    payload.insert(payload.end(), TEST_DEVICE_ID.begin(), TEST_DEVICE_ID.end()); // 4 bytes
    payload.insert(payload.end(), TEST_DEVICE_PUBLIC_KEY.begin(), TEST_DEVICE_PUBLIC_KEY.end()); // 64 bytes
    std::vector<byte> counter = RadioMeshUtils::numberToBytes(static_cast<uint32_t>(0)); // 4 bytes
    payload.insert(payload.end(), counter.begin(), counter.end());
    
    TEST_ASSERT_EQUAL(72, payload.size()); // Verify unencrypted payload size
    
    // INCLUDE_REQUEST should be direct ECC encrypted for standard device in pending state
    std::vector<byte> encrypted = service.encrypt(payload, MessageTopic::INCLUDE_REQUEST,
                                                 MeshDeviceType::STANDARD,
                                                 DeviceInclusionState::INCLUSION_PENDING);
    
    // Should be direct ECC encrypted: original 72 bytes with zero overhead = 72 bytes
    TEST_ASSERT_EQUAL(72, encrypted.size());
    TEST_ASSERT_FALSE(memcmp(payload.data(), encrypted.data(), 72) == 0);
}

void test_encryption_service_include_response() {
    EncryptionService service;
    service.setTempDevicePublicKey(TEST_DEVICE_PUBLIC_KEY);
    service.setDeviceKeys(TEST_HUB_PRIVATE_KEY, TEST_HUB_PUBLIC_KEY);
    
    // Build INCLUDE_RESPONSE payload: hub key + encrypted network key + nonce
    std::vector<byte> payload;
    payload.insert(payload.end(), TEST_HUB_PUBLIC_KEY.begin(), TEST_HUB_PUBLIC_KEY.end()); // 64 bytes
    std::vector<byte> encryptedNetworkKey(32, 0xAB); // Mock 32-byte encrypted key (direct ECC)
    payload.insert(payload.end(), encryptedNetworkKey.begin(), encryptedNetworkKey.end()); // 32 bytes
    std::vector<byte> nonce = {0xFF, 0xFE, 0xFD, 0xFC}; // 4 bytes
    payload.insert(payload.end(), nonce.begin(), nonce.end());
    
    TEST_ASSERT_EQUAL(100, payload.size()); // Verify unencrypted payload size
    
    // INCLUDE_RESPONSE should be direct ECC encrypted for hub
    std::vector<byte> encrypted = service.encrypt(payload, MessageTopic::INCLUDE_RESPONSE,
                                                 MeshDeviceType::HUB,
                                                 DeviceInclusionState::INCLUDED);
    
    // Should be direct ECC encrypted: original 100 bytes with zero overhead = 100 bytes
    TEST_ASSERT_EQUAL(100, encrypted.size());
    TEST_ASSERT_FALSE(memcmp(payload.data(), encrypted.data(), 100) == 0);
}

void test_encryption_service_include_confirm() {
    EncryptionService service;
    service.setNetworkKey(TEST_NETWORK_KEY);
    
    // INCLUDE_CONFIRM payload: incremented nonce (4 bytes)
    std::vector<byte> nonce = {0xFF, 0xFE, 0xFD, 0xFD}; // Original + 1
    
    // INCLUDE_CONFIRM should be AES encrypted for standard device in pending state
    std::vector<byte> encrypted = service.encrypt(nonce, MessageTopic::INCLUDE_CONFIRM,
                                                 MeshDeviceType::STANDARD,
                                                 DeviceInclusionState::INCLUSION_PENDING);
    
    // Should be AES encrypted: CTR mode outputs same size as input (4 bytes)
    TEST_ASSERT_EQUAL(4, encrypted.size());
    TEST_ASSERT_FALSE(memcmp(nonce.data(), encrypted.data(), 4) == 0);
    
    // NOW TEST DECRYPTION - This is what was missing!
    EncryptionService hubService;
    hubService.setNetworkKey(TEST_NETWORK_KEY);
    std::vector<byte> decrypted = hubService.decrypt(encrypted, MessageTopic::INCLUDE_CONFIRM,
                                                     MeshDeviceType::HUB,
                                                     DeviceInclusionState::INCLUDED);
    
    // Decrypted should match original
    TEST_ASSERT_EQUAL(nonce.size(), decrypted.size());
    TEST_ASSERT_EQUAL_MEMORY(nonce.data(), decrypted.data(), nonce.size());
}

void test_encryption_service_include_success() {
    EncryptionService service;
    service.setNetworkKey(TEST_NETWORK_KEY);
    
    // INCLUDE_SUCCESS payload: empty
    std::vector<byte> empty;
    
    // INCLUDE_SUCCESS should be AES encrypted for hub
    std::vector<byte> encrypted = service.encrypt(empty, MessageTopic::INCLUDE_SUCCESS,
                                                 MeshDeviceType::HUB,
                                                 DeviceInclusionState::INCLUDED);
    
    // Should be AES encrypted: CTR mode with empty input outputs empty
    TEST_ASSERT_EQUAL(0, encrypted.size());
}

// ========================================
// PAYLOAD CONSTRUCTION TESTS
// ========================================

void test_include_request_payload_construction() {
    // Test the exact payload that should be built for INCLUDE_REQUEST
    std::vector<byte> expectedPayload;
    
    // Device ID (4 bytes)
    expectedPayload.insert(expectedPayload.end(), TEST_DEVICE_ID.begin(), TEST_DEVICE_ID.end());
    
    // Device public key (32 bytes)
    expectedPayload.insert(expectedPayload.end(), TEST_DEVICE_PUBLIC_KEY.begin(), TEST_DEVICE_PUBLIC_KEY.end());
    
    // Initial counter (4 bytes) - should be 0 for new devices
    uint32_t initialCounter = 0;
    std::vector<byte> counterBytes = RadioMeshUtils::numberToBytes(initialCounter);
    expectedPayload.insert(expectedPayload.end(), counterBytes.begin(), counterBytes.end());
    
    // Verify total size
    TEST_ASSERT_EQUAL(72, expectedPayload.size());
    
    // Verify components
    TEST_ASSERT_EQUAL_MEMORY(TEST_DEVICE_ID.data(), expectedPayload.data(), 4);
    TEST_ASSERT_EQUAL_MEMORY(TEST_DEVICE_PUBLIC_KEY.data(), expectedPayload.data() + 4, 64);
    
    // Verify counter is 0
    uint32_t extractedCounter = RadioMeshUtils::bytesToNumber<uint32_t>(
        std::vector<byte>(expectedPayload.begin() + 68, expectedPayload.end()));
    TEST_ASSERT_EQUAL(0, extractedCounter);
}

void test_include_response_payload_construction() {
    // Test the exact payload that should be built for INCLUDE_RESPONSE
    std::vector<byte> expectedPayload;
    
    // Hub public key (64 bytes)
    expectedPayload.insert(expectedPayload.end(), TEST_HUB_PUBLIC_KEY.begin(), TEST_HUB_PUBLIC_KEY.end());
    
    // Mock encrypted network key (32 bytes - direct ECC output for 32-byte key)
    std::vector<byte> encryptedNetworkKey(32, 0xAB);
    expectedPayload.insert(expectedPayload.end(), encryptedNetworkKey.begin(), encryptedNetworkKey.end());
    
    // Nonce (4 bytes)
    std::vector<byte> nonce = {0xFF, 0xFE, 0xFD, 0xFC};
    expectedPayload.insert(expectedPayload.end(), nonce.begin(), nonce.end());
    
    // Verify total size
    TEST_ASSERT_EQUAL(100, expectedPayload.size());
    
    // Verify components can be extracted
    std::vector<byte> extractedHubKey(expectedPayload.begin(), expectedPayload.begin() + 64);
    TEST_ASSERT_EQUAL_MEMORY(TEST_HUB_PUBLIC_KEY.data(), extractedHubKey.data(), 64);
    
    std::vector<byte> extractedEncryptedKey(expectedPayload.begin() + 64, expectedPayload.begin() + 96);
    TEST_ASSERT_EQUAL(32, extractedEncryptedKey.size());
    
    std::vector<byte> extractedNonce(expectedPayload.begin() + 96, expectedPayload.end());
    TEST_ASSERT_EQUAL_MEMORY(nonce.data(), extractedNonce.data(), 4);
}

// ========================================
// DIRECT ECC ENCRYPTION/DECRYPTION TESTS
// ========================================

void test_direct_ecc_encryption_decryption() {
    EncryptionService service;
    service.setTempDevicePublicKey(TEST_DEVICE_PUBLIC_KEY);
    service.setDeviceKeys(TEST_HUB_PRIVATE_KEY, TEST_HUB_PUBLIC_KEY);
    
    // Test data
    std::vector<byte> originalData = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    // Encrypt using INCLUDE_RESPONSE context (which uses direct ECC)
    std::vector<byte> encrypted = service.encrypt(originalData, MessageTopic::INCLUDE_RESPONSE,
                                                 MeshDeviceType::HUB, DeviceInclusionState::INCLUDED);
    
    // Should have same size as original (zero overhead with direct ECC)
    TEST_ASSERT_EQUAL(originalData.size(), encrypted.size());
    TEST_ASSERT_FALSE(memcmp(originalData.data(), encrypted.data(), originalData.size()) == 0);
    
    // Test decryption - set up service as device receiving from hub
    EncryptionService deviceService;
    deviceService.setHubPublicKey(TEST_HUB_PUBLIC_KEY); // Device knows hub's public key
    std::vector<byte> decrypted = deviceService.decryptDirectECC(encrypted, TEST_DEVICE_PRIVATE_KEY);
    
    // Should recover original data
    TEST_ASSERT_EQUAL(originalData.size(), decrypted.size());
    TEST_ASSERT_EQUAL_MEMORY(originalData.data(), decrypted.data(), originalData.size());
}

// ========================================
// EXPECTED FAILURE TESTS
// ========================================

void test_include_request_without_hub_key_should_fail() {
    EncryptionService service;
    // Don't set hub public key
    
    std::vector<byte> payload(40, 0x01);
    
    // Should return original data when no key is available
    std::vector<byte> result = service.encrypt(payload, MessageTopic::INCLUDE_REQUEST,
                                              MeshDeviceType::STANDARD,
                                              DeviceInclusionState::INCLUSION_PENDING);
    
    // When encryption fails, should return original data
    TEST_ASSERT_EQUAL(40, result.size());
    TEST_ASSERT_EQUAL_MEMORY(payload.data(), result.data(), 40);
}

void test_include_response_without_device_key_should_fail() {
    EncryptionService service;
    // Don't set device public key
    
    std::vector<byte> payload(120, 0x02);
    
    // Should return original data when no key is available
    std::vector<byte> result = service.encrypt(payload, MessageTopic::INCLUDE_RESPONSE,
                                              MeshDeviceType::HUB,
                                              DeviceInclusionState::INCLUDED);
    
    // When encryption fails, should return original data
    TEST_ASSERT_EQUAL(120, result.size());
    TEST_ASSERT_EQUAL_MEMORY(payload.data(), result.data(), 120);
}

void test_aes_encryption_without_network_key_should_fail() {
    EncryptionService service;
    // Don't set network key
    
    std::vector<byte> payload = {0xFF, 0xFE, 0xFD, 0xFC};
    
    // Should return original data when no key is available
    std::vector<byte> result = service.encrypt(payload, MessageTopic::INCLUDE_CONFIRM,
                                              MeshDeviceType::STANDARD,
                                              DeviceInclusionState::INCLUSION_PENDING);
    
    // When encryption fails, should return original data
    TEST_ASSERT_EQUAL(4, result.size());
    TEST_ASSERT_EQUAL_MEMORY(payload.data(), result.data(), 4);
}

void test_direct_ecc_decryption_with_invalid_size_should_fail() {
    EncryptionService service;
    
    // Invalid direct ECC data (too small)
    std::vector<byte> invalidData = {0x01, 0x02, 0x03};
    
    std::vector<byte> result = service.decryptDirectECC(invalidData, TEST_DEVICE_PRIVATE_KEY);
    
    // Should return original data when decryption fails
    TEST_ASSERT_EQUAL(invalidData.size(), result.size());
    TEST_ASSERT_EQUAL_MEMORY(invalidData.data(), result.data(), invalidData.size());
}

// ========================================
// PROTOCOL STATE MACHINE TESTS
// ========================================

void test_encryption_method_determination() {
    EncryptionService service;
    
    // Test INCLUDE_OPEN - should always be NONE
    auto openEncrypted = service.encrypt({0x01}, MessageTopic::INCLUDE_OPEN, 
                                        MeshDeviceType::HUB, DeviceInclusionState::INCLUDED);
    TEST_ASSERT_EQUAL(1, openEncrypted.size()); // No encryption
    
    // Test INCLUDE_REQUEST - should be direct ECC for standard device in pending state
    service.setHubPublicKey(TEST_HUB_PUBLIC_KEY);
    auto requestEncrypted = service.encrypt({0x01}, MessageTopic::INCLUDE_REQUEST,
                                           MeshDeviceType::STANDARD, DeviceInclusionState::INCLUSION_PENDING);
    TEST_ASSERT_EQUAL(1, requestEncrypted.size()); // Direct ECC encryption (zero overhead)
    
    // Test INCLUDE_REQUEST - should be NONE for hub or non-pending device
    auto requestHub = service.encrypt({0x01}, MessageTopic::INCLUDE_REQUEST,
                                     MeshDeviceType::HUB, DeviceInclusionState::INCLUDED);
    TEST_ASSERT_EQUAL(1, requestHub.size()); // No encryption for hub
    
    auto requestIncluded = service.encrypt({0x01}, MessageTopic::INCLUDE_REQUEST,
                                          MeshDeviceType::STANDARD, DeviceInclusionState::INCLUDED);
    TEST_ASSERT_EQUAL(1, requestIncluded.size()); // No encryption for included device
}

void test_nonce_increment_logic() {
    // Test nonce increment for INCLUDE_CONFIRM
    std::vector<byte> originalNonce = {0xFF, 0xFE, 0xFD, 0xFC};
    uint32_t originalValue = RadioMeshUtils::bytesToNumber<uint32_t>(originalNonce);
    
    uint32_t incrementedValue = originalValue + 1;
    std::vector<byte> incrementedNonce = RadioMeshUtils::numberToBytes(incrementedValue);
    
    // Verify increment
    TEST_ASSERT_EQUAL(originalValue + 1, incrementedValue);
    TEST_ASSERT_FALSE(memcmp(originalNonce.data(), incrementedNonce.data(), 4) == 0);
    
    // Verify we can extract the incremented value
    uint32_t extracted = RadioMeshUtils::bytesToNumber<uint32_t>(incrementedNonce);
    TEST_ASSERT_EQUAL(incrementedValue, extracted);
}

// ========================================
// REAL WORLD SCENARIO TESTS
// ========================================

void test_real_world_nonce_encryption_decryption() {
    // Test the exact scenario from the logs
    // Original nonce: 0x05010101 (83951873)
    // Incremented: 0x05010102 (83951874)
    
    std::vector<byte> originalNonce = {0x05, 0x01, 0x01, 0x01};
    std::vector<byte> incrementedNonce = {0x05, 0x01, 0x01, 0x02};
    
    // Device encrypts the incremented nonce
    EncryptionService deviceService;
    deviceService.setNetworkKey(TEST_NETWORK_KEY);
    std::vector<byte> encrypted = deviceService.encrypt(incrementedNonce, MessageTopic::INCLUDE_CONFIRM,
                                                        MeshDeviceType::STANDARD,
                                                        DeviceInclusionState::INCLUSION_PENDING);
    
    // Hub decrypts the nonce
    EncryptionService hubService;
    hubService.setNetworkKey(TEST_NETWORK_KEY);
    std::vector<byte> decrypted = hubService.decrypt(encrypted, MessageTopic::INCLUDE_CONFIRM,
                                                     MeshDeviceType::HUB,
                                                     DeviceInclusionState::INCLUDED);
    
    // Verify decryption worked
    TEST_ASSERT_EQUAL(incrementedNonce.size(), decrypted.size());
    TEST_ASSERT_EQUAL_MEMORY(incrementedNonce.data(), decrypted.data(), incrementedNonce.size());
    
    // Verify the actual values
    uint32_t decryptedValue = RadioMeshUtils::bytesToNumber<uint32_t>(decrypted);
    TEST_ASSERT_EQUAL(83951874, decryptedValue); // 0x05010102
}

// ========================================
// TEST RUNNER
// ========================================

void setup() {
    UNITY_BEGIN();
    
    // Encryption Service Tests
    RUN_TEST(test_encryption_service_include_open);
    RUN_TEST(test_encryption_service_include_request);
    RUN_TEST(test_encryption_service_include_response);
    RUN_TEST(test_encryption_service_include_confirm);
    RUN_TEST(test_encryption_service_include_success);
    
    // Payload Construction Tests
    RUN_TEST(test_include_request_payload_construction);
    RUN_TEST(test_include_response_payload_construction);
    
    // Direct ECC Tests
    RUN_TEST(test_direct_ecc_encryption_decryption);
    
    // Expected Failure Tests
    RUN_TEST(test_include_request_without_hub_key_should_fail);
    RUN_TEST(test_include_response_without_device_key_should_fail);
    RUN_TEST(test_aes_encryption_without_network_key_should_fail);
    RUN_TEST(test_direct_ecc_decryption_with_invalid_size_should_fail);
    
    // Protocol Logic Tests
    RUN_TEST(test_encryption_method_determination);
    RUN_TEST(test_nonce_increment_logic);
    
    // Real World Scenario Tests
    RUN_TEST(test_real_world_nonce_encryption_decryption);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework
}