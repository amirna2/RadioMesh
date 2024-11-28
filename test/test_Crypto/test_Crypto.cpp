#include <unity.h>
#include "ICrypto.h"
#include "AesCrypto.h"

std::vector<byte> key = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                        };

std::vector<byte> iv = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};




void test_encrypt_decrypt(void)
{
      AesCrypto *crypto = AesCrypto::getInstance();

      crypto->setKey(key);
      crypto->setIV(iv);
      std::vector<byte> clearData = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                     0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
      std::vector<byte> encryptedData = crypto->encrypt(clearData);
      std::vector<byte> decryptedData = crypto->decrypt(encryptedData);

      TEST_ASSERT_EQUAL(clearData.size(), decryptedData.size());
      for (size_t i = 0; i < clearData.size(); i++) {
          TEST_ASSERT_EQUAL(clearData[i], decryptedData[i]);
      }

}
void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_encrypt_decrypt);
    UNITY_END();
}


void loop()
{
}