#include <RadioMesh.h>
#include <unity.h>

auto storage = EEPROMStorage::getInstance();

EEPROMStorage* initStorage(int size = 128)
{
    TEST_ASSERT_NOT_NULL(storage);

    ByteStorageParams params0(0);
    TEST_ASSERT_EQUAL(RM_E_STORAGE_INVALID_SIZE, storage->setParams(params0));
    ByteStorageParams params1025(1025);
    TEST_ASSERT_EQUAL(RM_E_STORAGE_INVALID_SIZE, storage->setParams(params1025));
    ByteStorageParams params(size);
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->setParams(params));
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->begin());
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_EQUAL(0, storage->getEntryCount());
    TEST_ASSERT_TRUE((storage->available() > 0));
    TEST_ASSERT_FALSE(storage->isFull());

    Serial.println("Storage initialized.....");
    return storage;
}

void resetEEPROMStorage()
{
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_EQUAL(0, storage->getEntryCount());
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->end());
}

void test_EEPROMStorage_write_exist_and_read(void)
{
    std::vector<byte> writeData = {1, 2, 3, 4};

    TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("test1", writeData));
    TEST_ASSERT_EQUAL(1, storage->getEntryCount());
    TEST_ASSERT_TRUE(storage->exists("test1"));

    std::vector<byte> readData;
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->read("test1", readData));
    TEST_ASSERT_EQUAL(writeData.size(), readData.size());
}

void test_EEPROMStorage_remove(void)
{
    std::vector<byte> writeData = {1, 2, 3, 4};

    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("test1", writeData));
    TEST_ASSERT_TRUE(storage->exists("test1"));
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->remove("test1"));
    TEST_ASSERT_FALSE(storage->exists("test1"));
}

void test_EEPROMStorage_clear(void)
{
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_EQUAL(0, storage->getEntryCount());

    std::vector<byte> writeData = {1, 2, 3, 4};
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("test1", writeData));
    TEST_ASSERT_EQUAL(1, storage->getEntryCount());
    TEST_ASSERT_TRUE(storage->exists("test1"));

    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_EQUAL(0, storage->getEntryCount());
    TEST_ASSERT_FALSE(storage->exists("test1"));
}

void test_EEPROMStorage_available(void)
{
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_TRUE((storage->available() > 0));
}

void test_EEPROMStorage_isFull(void)
{
    // start with a fresh storage
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());

    int size = storage->available();
    std::string key = "akey";

    // overhead is the size of the following:
    // - EntryHeader size
    // - Key length ("akey")
    // - Any alignment padding
    const int overhead = 10;

    std::vector<byte> writeData(size - overhead, 'a');

    TEST_ASSERT_TRUE(size > 0);
    TEST_ASSERT_FALSE(storage->isFull());

    TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("akey", writeData));
    TEST_ASSERT_EQUAL(1, storage->getEntryCount());
    TEST_ASSERT_TRUE(storage->exists("akey"));
    TEST_ASSERT_TRUE(storage->isFull());
    TEST_ASSERT_EQUAL(0, storage->available());
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
}

void test_EEPROMStorage_read_nonexistent(void)
{
    std::vector<byte> readData;
    TEST_ASSERT_EQUAL(RM_E_STORAGE_KEY_NOT_FOUND, storage->read("nonexistent", readData));
}

void test_EEPROMStorage_read_empty_key(void)
{
    std::vector<byte> readData;
    TEST_ASSERT_EQUAL(RM_E_INVALID_PARAM, storage->read("", readData));
}

void test_EEPROMStorage_write_empty_key(void)
{
    std::vector<byte> writeData = {1, 2, 3, 4};
    TEST_ASSERT_EQUAL(RM_E_INVALID_PARAM, storage->write("", writeData));
}

void test_EEPROMStorage_write_empty_data(void)
{
    std::vector<byte> emptyData;
    TEST_ASSERT_EQUAL(RM_E_INVALID_PARAM, storage->write("test", emptyData));
}

void test_EEPROMStorage_write_too_long_key(void)
{
    std::string longKey(256, 'a'); // 256 byte key
    std::vector<byte> data = {1};
    TEST_ASSERT_EQUAL(RM_E_INVALID_LENGTH, storage->write(longKey, data));
}

void test_EEPROMStorage_write_multiple(void)
{
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
    TEST_ASSERT_EQUAL(RM_E_STORAGE_KEY_NOT_FOUND, storage->remove("nonexistent"));
}

void test_EEPROMStorage_write_too_long_data(void)
{
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    std::string longData(129, 'a');
    TEST_ASSERT_EQUAL(RM_E_STORAGE_NOT_ENOUGH_SPACE,
                      storage->write("test", std::vector<byte>(longData.begin(), longData.end())));
}

void test_EEPROMStorage_commit(void)
{
    std::vector<byte> data = {1, 2, 3, 4};
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->write("test", data));
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->commit());
    TEST_ASSERT_TRUE(storage->exists("test"));
}

void test_EEPROMStorage_write_and_commit(void)
{
    std::vector<byte> data = {1, 2, 3, 4};
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->writeAndCommit("test", data));
    TEST_ASSERT_TRUE(storage->exists("test"));
}

void test_EEPROMStorage_write_and_commit_same_key(void)
{
    std::vector<byte> data1 = {1, 2, 3, 4};
    std::vector<byte> data2 = {5, 6, 7, 8};
    std::vector<byte> readData;

    TEST_ASSERT_EQUAL(RM_E_NONE, storage->clear());
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->writeAndCommit("test", data1));
    TEST_ASSERT_TRUE(storage->exists("test"));
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->read("test", readData));
    TEST_ASSERT_EQUAL(4, readData.size());
    TEST_ASSERT_EQUAL(1, readData[0]);
    TEST_ASSERT_EQUAL(2, readData[1]);
    TEST_ASSERT_EQUAL(3, readData[2]);
    TEST_ASSERT_EQUAL(4, readData[3]);

    TEST_ASSERT_EQUAL(RM_E_NONE, storage->writeAndCommit("test", data2));
    TEST_ASSERT_TRUE(storage->exists("test"));
    TEST_ASSERT_EQUAL(RM_E_NONE, storage->read("test", readData));
    TEST_ASSERT_EQUAL(4, readData.size());
    TEST_ASSERT_EQUAL(5, readData[0]);
    TEST_ASSERT_EQUAL(6, readData[1]);
    TEST_ASSERT_EQUAL(7, readData[2]);
    TEST_ASSERT_EQUAL(8, readData[3]);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ; // Wait for serial connection (optional)

    initStorage();
    UNITY_BEGIN();
    Serial.println("EEPROMStorage test_EEPROMStorage_write_exist_and_read");
    RUN_TEST(test_EEPROMStorage_write_exist_and_read);
    Serial.println("test_EEPROMStorage_remove tests");
    RUN_TEST(test_EEPROMStorage_remove);
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
    RUN_TEST(test_EEPROMStorage_commit);
    RUN_TEST(test_EEPROMStorage_write_and_commit);
    RUN_TEST(test_EEPROMStorage_write_and_commit_same_key);

    resetEEPROMStorage();
    UNITY_END();
}

void loop()
{
}
