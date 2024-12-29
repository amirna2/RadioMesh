#include <RadioMesh.h>
#include <unity.h>

WifiAccessPointParams apParams = {"WarpAP", "WarpAP123", "192.168.1.1"};

// positive test cases
void test_WifiAccessPoint_getInstance(void)
{
    WifiAccessPoint* ap = WifiAccessPoint::getInstance();
    TEST_ASSERT_NOT_NULL(ap);
}

void test_WifiAccessPoint_setParams(void)
{
    WifiAccessPoint* ap = WifiAccessPoint::getInstance();
    int rc = ap->setParams(apParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
    rc = ap->setup();
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_WifiAccessPoint_start(void)
{
    WifiAccessPoint* ap = WifiAccessPoint::getInstance();
    int rc = ap->start();
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
    bool isRunning = ap->isStarted();
    TEST_ASSERT_TRUE(isRunning);
}

void test_WifiAccessPoint_stop(void)
{
    WifiAccessPoint* ap = WifiAccessPoint::getInstance();
    int rc = ap->stop(false);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
    bool isRunning = ap->isStarted();
    TEST_ASSERT_FALSE(isRunning);
}

// negative test cases
void test_WifiAccessPoint_setParams_with_invalid_ssid_length(void)
{
    WifiAccessPoint* ap = WifiAccessPoint::getInstance();
    std::string longSsid(65, 'a');
    WifiAccessPointParams apParams = {longSsid, "WarpAP123", "192.168.1.1"}; // SSID is too long
    int rc = ap->setParams(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
}

void test_WifiAccessPoint_setParams_with_invalid_password_length(void)
{
    WifiAccessPoint* ap = WifiAccessPoint::getInstance();
    std::string longPassword(1, 'a');
    WifiAccessPointParams apParams = {"WarpAP", longPassword,
                                      "192.168.1.1"}; // Password is too short
    int rc = ap->setParams(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
}

void test_WifiAccessPoint_setParams_with_empty_params(void)
{
    WifiAccessPoint* ap = WifiAccessPoint::getInstance();
    WifiAccessPointParams apParams = {"", "password1234", "192.168.1.1"};
    int rc = ap->setParams(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
    apParams = {"wifiaccesspoint_ssid", "", "192.168.1.1"};
    rc = ap->setParams(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
    apParams = {"wifiaccesspoint_ssid", "password1234", ""};
    rc = ap->setParams(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
    apParams = {"wifiaccesspoint_ssid", "password1234", "1234.56.7.8"};
    rc = ap->setParams(apParams);
    TEST_ASSERT_EQUAL(RM_E_INVALID_AP_PARAMS, rc);
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_WifiAccessPoint_getInstance);
    RUN_TEST(test_WifiAccessPoint_setParams);
    RUN_TEST(test_WifiAccessPoint_start);
    RUN_TEST(test_WifiAccessPoint_stop);
    RUN_TEST(test_WifiAccessPoint_setParams_with_invalid_ssid_length);
    RUN_TEST(test_WifiAccessPoint_setParams_with_invalid_password_length);
    RUN_TEST(test_WifiAccessPoint_setParams_with_empty_params);
    UNITY_END();
}

void loop()
{
    // Empty loop
}
