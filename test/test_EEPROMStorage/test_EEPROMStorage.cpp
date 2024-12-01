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
}

void test_EEPROMStorage_begin(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->begin());
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
   TEST_ASSERT_EQUAL(0, storage->getEntryCount());
}

void test_EEPROMStorage_write_exist_and_read(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::vector<byte> writeData = {1, 2, 3, 4};
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("test", writeData));
   TEST_ASSERT_EQUAL(1, storage->getEntryCount());
   TEST_ASSERT_TRUE(storage->exists("test"));

   std::vector<byte> readData;
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->read("test", readData));
   TEST_ASSERT_EQUAL(writeData.size(), readData.size());
}

void test_EEPROMStorage_remove(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::vector<byte> writeData = {1, 2, 3, 4};

   TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("test", writeData));
   TEST_ASSERT_TRUE(storage->exists("test"));
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->remove("test"));
   TEST_ASSERT_FALSE(storage->exists("test"));
}

void test_EEPROMStorage_clear(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
   TEST_ASSERT_EQUAL(0, storage->getEntryCount());
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

void test_EEPROMStorage_read_nonexistent(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::vector<byte> readData;
   TEST_ASSERT_EQUAL(RM_E_STORAGE_KEY_NOT_FOUND, storage->read("nonexistent", readData));
}

void test_EEPROMStorage_read_empty_key(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::vector<byte> readData;
   TEST_ASSERT_EQUAL(RM_E_INVALID_PARAM, storage->read("", readData));
}

void test_EEPROMStorage_write_empty_key(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::vector<byte> writeData = {1, 2, 3, 4};
   TEST_ASSERT_EQUAL(RM_E_INVALID_PARAM, storage->write("", writeData));
}

void test_EEPROMStorage_write_empty_data(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::vector<byte> emptyData;
   TEST_ASSERT_EQUAL(RM_E_INVALID_PARAM, storage->write("test", emptyData));
}

void test_EEPROMStorage_write_too_long_key(void)
{
   auto storage = EEPROMStorage::getInstance();
   std::string longKey(256, 'a');  // 256 byte key
   std::vector<byte> data = {1};
   TEST_ASSERT_EQUAL(RM_E_INVALID_LENGTH, storage->write(longKey, data));
}

void test_EEPROMStorage_write_multiple(void)
{
   auto storage = EEPROMStorage::getInstance();

   std::vector<byte> data1 = {1, 2};
   std::vector<byte> data2 = {3, 4};
   std::vector<byte> readData;

   TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());

   TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("key1", data1));
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("key2", data2));

   TEST_ASSERT_EQUAL(RM_E_NONE, storage->read("key1", readData));
   TEST_ASSERT_EQUAL(2, readData.size());
   TEST_ASSERT_EQUAL(1, readData[0]);
   TEST_ASSERT_EQUAL(2, readData[1]);

   TEST_ASSERT_EQUAL(RM_E_NONE, storage->read("key2", readData));
   TEST_ASSERT_EQUAL(2, readData.size());
   TEST_ASSERT_EQUAL(3, readData[0]);
   TEST_ASSERT_EQUAL(4, readData[1]);
}

void test_EEPROMStorage_remove_nonexistent(void)
{
   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_EQUAL(RM_E_STORAGE_KEY_NOT_FOUND, storage->remove("nonexistent"));
}

void test_EEPROMStorage_write_too_long_data(void)
{
   StorageParams params;
   params.size = 1024;  // 1KB storage

   auto storage = EEPROMStorage::getInstance();
   TEST_ASSERT_NOT_NULL(storage);
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->setParams(params));
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
   TEST_ASSERT_EQUAL(RM_E_NONE, storage->begin());
   std::string longData(1025, 'a');  // 1025 byte data
   TEST_ASSERT_EQUAL(RM_E_STORAGE_NOT_ENOUGH_SPACE, storage->write("test", std::vector<byte>(longData.begin(), longData.end())));
}

void setup()
{
   Serial.begin(115200);
   while (!Serial);  // Wait for serial connection (optional)

   UNITY_BEGIN();
   RUN_TEST(test_EEPROMStorage_setParams);
   RUN_TEST(test_EEPROMStorage_begin);
   RUN_TEST(test_EEPROMStorage_write_exist_and_read);
   RUN_TEST(test_EEPROMStorage_remove); // remove key from previous test
   RUN_TEST(test_EEPROMStorage_write_multiple);
   RUN_TEST(test_EEPROMStorage_clear);
   RUN_TEST(test_EEPROMStorage_available);
   RUN_TEST(test_EEPROMStorage_isFull);
   RUN_TEST(test_EEPROMStorage_read_nonexistent);
   RUN_TEST(test_EEPROMStorage_read_empty_key);
   RUN_TEST(test_EEPROMStorage_write_empty_key);
   RUN_TEST(test_EEPROMStorage_write_empty_data);
   RUN_TEST(test_EEPROMStorage_write_too_long_key);
   RUN_TEST(test_EEPROMStorage_remove_nonexistent);
   RUN_TEST(test_EEPROMStorage_write_too_long_data);
   RUN_TEST(test_EEPROMStorage_end);
   UNITY_END();
}

void loop()
{
}
