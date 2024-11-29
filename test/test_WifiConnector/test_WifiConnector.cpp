#include <unity.h>
#include <RadioMesh.h>
#include "wifiConfig.h"

WifiParams wifiParams = {WIFI_SSID, WIFI_PASSWORD};

// positive test cases

void test_WifiConnector_getInstance(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   TEST_ASSERT_NOT_NULL(wifi);
}

void test_WifiConnector_setParams(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   int rc = wifi->setParams(wifiParams);
   TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_WifiConnector_connect(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   int rc = wifi->setParams(wifiParams);
   TEST_ASSERT_EQUAL(RM_E_NONE, rc);
   rc = wifi->connect();
   TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_WifiConnector_reconnect(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   int rc = wifi->setParams(wifiParams);
   TEST_ASSERT_EQUAL(RM_E_NONE, rc);
   rc = wifi->reconnect();
   TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_WifiConnector_getIpAddress(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   std::string ip = wifi->getIpAddress();
   TEST_ASSERT_TRUE(ip.length() > 0);
}

void test_WifiConnector_getMacAddress(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   std::string mac = wifi->getMacAddress();
   TEST_ASSERT_TRUE(mac.length() > 0);
}

void test_WifiConnector_getSignalIndicator(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   SignalIndicator indicator = wifi->getSignalIndicator();
   TEST_ASSERT_TRUE(indicator != NO_SIGNAL);
}

void test_WifiConnector_getSignalStrength(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   int strength = wifi->getSignalStrength();
   TEST_ASSERT_TRUE(strength < 0);
}

void test_WifiConnector_getSSID(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   std::string ssid = wifi->getSSID();
   TEST_ASSERT_TRUE(ssid.length() > 0);
}

void test_WifiConnector_getAvailableNetworks(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   std::vector<std::string> networks = wifi->getAvailableNetworks();
   TEST_ASSERT_TRUE(networks.size() > 0);
}

void test_WifiConnector_disconnect(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   int rc = wifi->disconnect(false);
   TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

// negative test cases
void test_WifiConnector_setParams_with_invalid_ssid_length(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   std::string longSsid(33, 'a');
   WifiParams wifiParams = {longSsid, "firefly2424"}; // SSID is too long
   int rc = wifi->setParams(wifiParams);
   TEST_ASSERT_EQUAL(RM_E_INVALID_WIFI_PARAMS, rc);
}
void test_WifiConnector_setParams_with_invalid_password_length(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   std::string longPassword(65, 'a');
   WifiParams wifiParams = {"my_test_wifi", longPassword}; // Password is too long
   int rc = wifi->setParams(wifiParams);
   TEST_ASSERT_EQUAL(RM_E_INVALID_WIFI_PARAMS, rc);
}

void test_WifiConnector_params_with_invalid_ssid(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   WifiParams wifiParams = {"", "firefly2424"}; // SSID is empty
   int rc = wifi->setParams(wifiParams);

   TEST_ASSERT_EQUAL(RM_E_INVALID_WIFI_PARAMS, rc);
}

void test_WifiConnector_params_with_invalid_password(void)
{
   WifiConnector *wifi = WifiConnector::getInstance();
   WifiParams wifiParams = {"WarpPortal", ""}; // Password is empty
   int rc = wifi->setParams(wifiParams);
   TEST_ASSERT_EQUAL(RM_E_INVALID_WIFI_PARAMS, rc);
}

void setup()
{
   UNITY_BEGIN();
   RUN_TEST(test_WifiConnector_getInstance);
   RUN_TEST(test_WifiConnector_setParams);
   RUN_TEST(test_WifiConnector_connect);
   RUN_TEST(test_WifiConnector_reconnect);
   RUN_TEST(test_WifiConnector_getIpAddress);
   RUN_TEST(test_WifiConnector_getMacAddress);
   RUN_TEST(test_WifiConnector_getSignalIndicator);
   RUN_TEST(test_WifiConnector_getSignalStrength);
   RUN_TEST(test_WifiConnector_getSSID);
   RUN_TEST(test_WifiConnector_getAvailableNetworks);
   RUN_TEST(test_WifiConnector_disconnect);
   RUN_TEST(test_WifiConnector_setParams_with_invalid_ssid_length);
   RUN_TEST(test_WifiConnector_setParams_with_invalid_password_length);
   RUN_TEST(test_WifiConnector_params_with_invalid_ssid);
   RUN_TEST(test_WifiConnector_params_with_invalid_password);
   UNITY_END();
}

void loop()
{
   // Empty loop
}
