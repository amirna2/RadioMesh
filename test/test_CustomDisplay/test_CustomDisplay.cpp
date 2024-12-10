#include "CustomDisplay.h"
#include <RadioMesh.h>
#include <unity.h>

const std::array<byte, DEV_ID_LENGTH> device_id = {0x11, 0x11, 0x11, 0x11};
const std::string DEVICE_NAME = "TestDevice";

IDevice* device = nullptr;

void reset_device()
{
   if (device != nullptr) {
      delete device;
   }
   device = nullptr;
}

void test_DeviceBuilder_buildDevice_withCustomDisplay(void)
{
   DeviceBuilder builder;
   reset_device();
   device = builder.start().withCustomDisplay(nullptr).withRelayEnabled(true).build(
       "test", device_id, MeshDeviceType::STANDARD);

   TEST_ASSERT_NOT_NULL(device->getDisplay());
}

void setup()
{
   UNITY_BEGIN();
   RUN_TEST(test_DeviceBuilder_buildDevice_withCustomDisplay);
   UNITY_END();
}

void loop()
{
}
