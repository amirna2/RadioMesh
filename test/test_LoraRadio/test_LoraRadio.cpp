#include <unity.h>
#include <Warp.h>
#include "LoraRadio.h"


// These radio parameters will only work for the Heltec WiFi LoRa 32 V3
LoraRadioParams radioParams = LoraRadioParams(8, 12, 13, 14, 915.0, 20, 125.0, 7, 0, true);

LoraRadio *radio = nullptr;

void test_LoraRadio_getInstance(void) {
   radio = LoraRadio::getInstance();
   TEST_ASSERT_NOT_NULL(radio);
}

void test_LoraRadio_setup(void) {
   int rc = radio->setup();
   TEST_ASSERT_EQUAL(WARP_ERR_NONE, rc);
   TEST_ASSERT_TRUE(radio->isRadioSetup());
   LoraRadioParams params = radio->getParams();
   TEST_ASSERT_EQUAL(radioParams.ss, params.ss);
   TEST_ASSERT_EQUAL(radioParams.rst, params.rst);
}

void test_LoraRadio_setup_with_given_radio_params(void) {
   LoraRadioParams radioParams = LoraRadioParams(8, 12, 13, 14, 915.0, 20, 125.0, 9, 0, true);
   int rc = radio->setup(radioParams);
   TEST_ASSERT_EQUAL(WARP_ERR_NONE, rc);
   TEST_ASSERT_TRUE(radio->isRadioSetup());
   LoraRadioParams params = radio->getParams();
   TEST_ASSERT_EQUAL(radioParams.sf, params.sf);
}


void setup() {
   UNITY_BEGIN();
   RUN_TEST(test_LoraRadio_getInstance);
   RUN_TEST(test_LoraRadio_setup);
   RUN_TEST(test_LoraRadio_setup_with_given_radio_params);
   UNITY_END();
}

void loop() {
    // Empty loop
}
