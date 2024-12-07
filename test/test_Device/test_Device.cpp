#include <unity.h>
#include <RadioMesh.h>

// These radio parameters will only work for the Heltec WiFi LoRa 32 V3
const LoraRadioParams  radioParams = LoraRadioParams(PinConfig(8, 12, 13, 14), 915.0, 20, 125.0, 8, 0, true);

// Radio parameters with invalid frequency
const LoraRadioParams badRadioParams1 = LoraRadioParams(PinConfig(8, 12, 13, 14), 0.0, 20, 125.0, 8, 0, true);

// Readio parameters with invalid spreading factor
LoraRadioParams badRadioParams2 = LoraRadioParams(PinConfig(8, 12, 13, 14), 915.0, 20, 125.0, 0, 0, true);

// AES key and IV
std::vector<byte> key = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                        };

std::vector<byte> iv = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                         0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                       };

std::array<byte, DEV_ID_LENGTH> id = {0x11, 0x11, 0x11, 0x11};
std::string deviceName = "testDevice";

RadioMeshDevice createDevice() {
    return RadioMeshDevice("testDevice", id);
}

void test_getDeviceId(void) {
    RadioMeshDevice device = createDevice();
    TEST_ASSERT_TRUE(std::equal(id.begin(), id.end(), device.getDeviceId().begin()));
}

void test_getDeviceName(void) {
    RadioMeshDevice device = createDevice();

    TEST_ASSERT_EQUAL_STRING("testDevice", device.getDeviceName().c_str());
}

void test_initializeRadio(void) {
    RadioMeshDevice device = createDevice();

    int rc = device.initializeRadio(radioParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_initializeRadio_bad_params(void) {
    RadioMeshDevice device = createDevice();

    // Test with invalid frequency
    int rc = device.initializeRadio(badRadioParams1);
    TEST_ASSERT_EQUAL(RM_E_INVALID_RADIO_PARAMS, rc);
    // Test with invalid spreading factor
    rc = device.initializeRadio(badRadioParams2);
    TEST_ASSERT_EQUAL(RM_E_INVALID_RADIO_PARAMS, rc);
}

void test_initializeAesCrypto(void) {
    RadioMeshDevice device = createDevice();

    int rc = device.initializeAesCrypto(key, iv);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_initializeOledDisplay(void) {
    RadioMeshDevice device = createDevice();
    OledDisplayParams displayParams = OledDisplayParams(SCL_OLED, SDA_OLED, RST_OLED, RM_FONT_SMALL);

    int rc = device.initializeOledDisplay(displayParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_initializeWifi(void) {
    RadioMeshDevice device = createDevice();
    WifiParams wifiParams = {"Portal", "firefly2424"};

    int rc = device.initializeWifi(wifiParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_initializeWifi_with_invalid_ssid_length(void) {
    RadioMeshDevice device = createDevice();
    std::string longSsid(33, 'a');
    WifiParams wifiParams = {longSsid, "firefly2424"};  // SSID is too long
    int rc = device.initializeWifi(wifiParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_WIFI_PARAMS, rc);
}

void test_initializeWifi_with_invalid_password_length(void) {
    RadioMeshDevice device = createDevice();
    std::string longPassword(65, 'a');
    WifiParams wifiParams = {"my_test_wifi", longPassword};  // Password is too long

    int rc = device.initializeWifi(wifiParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_WIFI_PARAMS, rc);
}

void test_initializeWifiAccessPoint(void) {
    RadioMeshDevice device = createDevice();
    WifiAccessPointParams apParams = {"WarpPortal", "firefly2424", "192.168.0.10"};
    int rc = device.initializeWifiAccessPoint(apParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_initializeWifiAccessPoint_with_invalid_ssid_length(void) {
    RadioMeshDevice device = createDevice();
    std::string longSsid(65, 'a');
    WifiAccessPointParams apParams = {longSsid, "firefly2424", "192.168.0.10"};  // SSID is too long
    int rc = device.initializeWifiAccessPoint(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
}

void test_initializeWifiAccessPoint_with_invalid_password_length(void) {
    RadioMeshDevice device = createDevice();
    std::string shortPassword(4, 'a');
    WifiAccessPointParams apParams = {"WarpPortal", shortPassword, "192.168.0.10"};  // Password is too short
    int rc = device.initializeWifiAccessPoint(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
}

void test_initializeWifiAccessPoint_with_invalid_ip_address(void) {
    RadioMeshDevice device = createDevice();
    std::string invalidIp = "1234.34.2.1";
    WifiAccessPointParams apParams = {"WarpPortal", "firefly2424", invalidIp};  // Invalid IP address
    int rc = device.initializeWifiAccessPoint(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_getDeviceId);
    RUN_TEST(test_getDeviceName);
    RUN_TEST(test_initializeRadio_bad_params);
    RUN_TEST(test_initializeRadio);
    RUN_TEST(test_initializeAesCrypto);
    RUN_TEST(test_initializeOledDisplay);
    RUN_TEST(test_initializeWifi);
    RUN_TEST(test_initializeWifi_with_invalid_ssid_length);
    RUN_TEST(test_initializeWifi_with_invalid_password_length);
    RUN_TEST(test_initializeWifiAccessPoint);
    RUN_TEST(test_initializeWifiAccessPoint_with_invalid_ssid_length);
    RUN_TEST(test_initializeWifiAccessPoint_with_invalid_password_length);
    RUN_TEST(test_initializeWifiAccessPoint_with_invalid_ip_address);
    UNITY_END();
}

void loop() {
    // Empty loop
}