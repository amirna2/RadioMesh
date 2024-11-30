#include <unity.h>
#include <RadioMesh.h>


#include <unity.h>
#include <RadioMesh.h>

void test_EEPROMStorage_setParams(void)
{
   StorageParams params;
   params.size = 1024;  // 1KB storage

   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_NOT_NULL(storage);
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->setParams(params));
   storage->clear();
}

void test_EEPROMStorage_begin(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_NOT_NULL(storage);
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->begin());
}

void test_EEPROMStorage_write(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::vector<byte> writeData = {1, 2, 3, 4};
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("test", writeData));
}

void test_EEPROMStorage_exists(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_TRUE(storage->exists("test"));
}


void test_EEPROMStorage_read(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::vector<byte> readData;
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->read("test", readData));
   TEST_ASSERT_EQUAL(4, readData.size());  // From previous write test
}

void test_EEPROMStorage_remove(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->remove("test"));
   TEST_ASSERT_FALSE(storage->exists("test"));
}

void test_EEPROMStorage_clear(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
}

void test_EEPROMStorage_available(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_TRUE(storage->available() > 0);
}

void test_EEPROMStorage_isFull(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_FALSE(storage->isFull());
}

void test_EEPROMStorage_end(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->end());
}

void setup()
{
   UNITY_BEGIN();
   RUN_TEST(test_EEPROMStorage_setParams);
   RUN_TEST(test_EEPROMStorage_begin);
   RUN_TEST(test_EEPROMStorage_write);
   RUN_TEST(test_EEPROMStorage_exists);
   RUN_TEST(test_EEPROMStorage_read);
   RUN_TEST(test_EEPROMStorage_remove);
   RUN_TEST(test_EEPROMStorage_clear);
   RUN_TEST(test_EEPROMStorage_available);
   RUN_TEST(test_EEPROMStorage_isFull);
   RUN_TEST(test_EEPROMStorage_end);

    UNITY_END();
}

void loop()
{
}
