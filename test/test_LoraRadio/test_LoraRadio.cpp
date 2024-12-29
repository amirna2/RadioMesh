#include <RadioMesh.h>
#include <unity.h>

// These radio parameters will only work for the Heltec WiFi LoRa 32 V3
const LoraRadioParams radioParams =
    LoraRadioParams(PinConfig(8, 12, 13, 14), 915.0, 20, 125.0, 8, 0, true);
const LoraRadioParams badRadioParams =
    LoraRadioParams(PinConfig(0, 0, 0, 0), 915.0, 20, 125.0, 8, 0, true);

LoraRadio* radio = nullptr;

void test_LoraRadio_getInstance(void)
{
    radio = LoraRadio::getInstance();
    TEST_ASSERT_NOT_NULL(radio);
    TEST_ASSERT_FALSE(radio->isRadioSetup());
}

void test_LoraRadio_setParams_valid_parameters(void)
{
    if (radio == nullptr) {
        radio = LoraRadio::getInstance();
    }
    // Get the default parameters
    LoraRadioParams defaultParams = radio->getParams();
    TEST_ASSERT_NOT_EQUAL(PinConfig::PIN_UNDEFINED, defaultParams.pinConfig.ss);
    TEST_ASSERT_NOT_EQUAL(PinConfig::PIN_UNDEFINED, defaultParams.pinConfig.di1);
    TEST_ASSERT_NOT_EQUAL(PinConfig::PIN_UNDEFINED, defaultParams.pinConfig.rst);
    TEST_ASSERT_NOT_EQUAL(PinConfig::PIN_UNDEFINED, defaultParams.pinConfig.di0);

    int rc = radio->setParams(radioParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
    LoraRadioParams newParams = radio->getParams();
    TEST_ASSERT_EQUAL(radioParams.pinConfig.ss, newParams.pinConfig.ss);
    TEST_ASSERT_EQUAL(radioParams.pinConfig.di1, newParams.pinConfig.di1);
    TEST_ASSERT_EQUAL(radioParams.pinConfig.rst, newParams.pinConfig.rst);
    TEST_ASSERT_EQUAL(radioParams.pinConfig.di0, newParams.pinConfig.di0);
}

void test_LoraRadio_setParams_bad_parameters(void)
{
    if (radio == nullptr) {
        radio = LoraRadio::getInstance();
    }
    // Get the default parameters
    LoraRadioParams defaultParams = radio->getParams();
    TEST_ASSERT_NOT_EQUAL(PinConfig::PIN_UNDEFINED, defaultParams.pinConfig.ss);
    TEST_ASSERT_NOT_EQUAL(PinConfig::PIN_UNDEFINED, defaultParams.pinConfig.di1);
    TEST_ASSERT_NOT_EQUAL(PinConfig::PIN_UNDEFINED, defaultParams.pinConfig.rst);
    TEST_ASSERT_NOT_EQUAL(PinConfig::PIN_UNDEFINED, defaultParams.pinConfig.di0);

    int rc = radio->setParams(badRadioParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
}

void test_LoraRadio_setup_without_set_params(void)
{
    if (radio == nullptr) {
        radio = LoraRadio::getInstance();
    }
    // This should fail because the radio parameters have not been set
    int rc = radio->setup();
    TEST_ASSERT_EQUAL(RM_E_INVALID_RADIO_PARAMS, rc);
}

void test_LoraRadio_setup_with_given_radio_params(void)
{
    if (radio == nullptr) {
        radio = LoraRadio::getInstance();
    }
    int rc = radio->setup(radioParams);
    TEST_ASSERT_EQUAL(RM_E_NONE, rc);
    TEST_ASSERT_TRUE(radio->isRadioSetup());
    LoraRadioParams params = radio->getParams();
    TEST_ASSERT_EQUAL(radioParams.sf, params.sf);
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_LoraRadio_getInstance);
    RUN_TEST(test_LoraRadio_setParams_valid_parameters);
    RUN_TEST(test_LoraRadio_setParams_bad_parameters);

    RUN_TEST(test_LoraRadio_setup_without_set_params);
    RUN_TEST(test_LoraRadio_setup_with_given_radio_params);
    UNITY_END();
}

void loop()
{
}
