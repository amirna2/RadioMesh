#include <unity.h>
#include <Warp.h>

// Default wifi configuration
WifiParams wifiParams = {"Lojika Systems 2.4G", "7212-Wabash!"};

LoraRadioParams defaultConfig = LoraRadioPresets::HELTEC_WIFI_LORA_32_V3;
// Default display configuration
OledDisplayParams displayParams = OledDisplayParams(SCL_OLED, SDA_OLED, RST_OLED, WARP_FONT_SMALL);
// Default access point configuration
WifiAccessPointParams apParams = {"WarpPortal", "firefly2424", "192.168.20.1"};

void RxCallback(const WarpPacket &packet, int error) {}

std::vector<byte> key = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                        };

std::vector<byte> iv = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                         0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                       };

std::vector<byte> device_id = {0xff, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
std::vector<byte> bad_device_id = {0xff, 0x11};


IDevice *device = nullptr;

void reset_device()
{
   if (device != nullptr) {
      delete device;
   }
   device = nullptr;
}

void test_DeviceBuilder_buildDevice_with_no_capabilities(void)
{
   reset_device();
   DeviceBuilder builder;
   device = builder.start()
                   .build("test", device_id);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
   TEST_ASSERT_FALSE(device->isRelayEnabled());
   TEST_ASSERT_NULL(device->getRadio());

}

void test_DeviceBuilder_buildDevice_with_bad_id(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .build("test", bad_device_id);

   TEST_ASSERT_NULL(device);

}


void test_DeviceBuilder_buildDevice_with_bad_name(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .build("", device_id);

   TEST_ASSERT_NULL(device);

   device = builder.start()
                   .build("123456789", device_id);

   TEST_ASSERT_NULL(device);
}



void test_DeviceBuilder_buildDevice_without_starting(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.build("test", device_id);
   TEST_ASSERT_NULL(device);
}


void test_DeviceBuilder_buildDevice_withRadio(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .withLoraRadio(defaultConfig)
                   .build("test", device_id, WarpDeviceType::ROUTER);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_NOT_NULL(device->getRadio());
   TEST_ASSERT_FALSE(device->isRelayEnabled());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
}

void test_DeviceBuilder_buildDevice_withRadio_and_withRelay(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .withLoraRadio(defaultConfig)
                   .withRelayEnabled(true)
                   .build("test", device_id, WarpDeviceType::ROUTER);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_TRUE(device->isRelayEnabled());
   TEST_ASSERT_NOT_NULL(device->getRadio());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
}

void test_DeviceBuilder_buildDevice_withWifi(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .withWifi(wifiParams)
                   .build("test", device_id, WarpDeviceType::ROUTER);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_NOT_NULL(device->getWifiConnector());
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
}
void test_DeviceBuilder_buildDevice_withWifiAccessPoint(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .withWifiAccessPoint(apParams)
                   .build("test", device_id, WarpDeviceType::ROUTER);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_NOT_NULL(device->getWifiAccessPoint());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
}

void test_DeviceBuilder_buildDevice_withRxCallback(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .withRxPacketCallback(RxCallback)
                   .build("test", device_id, WarpDeviceType::ROUTER);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
}

void test_DeviceBuilder_buildDevice_withAesCrypto(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .withAesCrypto(key, iv)
                   .build("test", device_id, WarpDeviceType::ROUTER);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
}
void test_DeviceBuilder_buildDevice_withOledDisplay(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .withOledDisplay(displayParams)
                   .build("test", device_id, WarpDeviceType::ROUTER);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
   TEST_ASSERT_NOT_NULL(device->getDisplay());
}
void test_DeviceBuilder_buildDevice_withEverything(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start()
                   .withLoraRadio(defaultConfig)
                   .withWifi(wifiParams)
                   .withWifiAccessPoint(apParams)
                   .withRelayEnabled(true)
                   .withRxPacketCallback(RxCallback)
                   .withAesCrypto(key, iv)
                   .withOledDisplay(displayParams)
                   .build("test", device_id, WarpDeviceType::ROUTER);

   TEST_ASSERT_NOT_NULL(device);
   TEST_ASSERT_EQUAL_STRING("test", device->getDeviceName().c_str());
   TEST_ASSERT_TRUE(device->isRelayEnabled());
   TEST_ASSERT_NOT_NULL(device->getRadio());
   TEST_ASSERT_EQUAL(8, device->getDeviceId().size());
}

void setup()
{
   UNITY_BEGIN();
   RUN_TEST(test_DeviceBuilder_buildDevice_without_starting);
   RUN_TEST(test_DeviceBuilder_buildDevice_with_no_capabilities);
   RUN_TEST(test_DeviceBuilder_buildDevice_withRadio);
   RUN_TEST(test_DeviceBuilder_buildDevice_withWifi);
   RUN_TEST(test_DeviceBuilder_buildDevice_withWifiAccessPoint);
   RUN_TEST(test_DeviceBuilder_buildDevice_with_bad_id);
   RUN_TEST(test_DeviceBuilder_buildDevice_with_bad_name);
   RUN_TEST(test_DeviceBuilder_buildDevice_withRadio_and_withRelay);
   RUN_TEST(test_DeviceBuilder_buildDevice_withRxCallback);
   RUN_TEST(test_DeviceBuilder_buildDevice_withAesCrypto);
   RUN_TEST(test_DeviceBuilder_buildDevice_withOledDisplay);
   RUN_TEST(test_DeviceBuilder_buildDevice_withEverything);

   UNITY_END();
}

void loop()
{
}
