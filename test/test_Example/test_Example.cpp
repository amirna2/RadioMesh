#include <unity.h>
#include "Logger.h"

void test_Loger_loginfo_ln(void)
{
   loginfo_ln("Hello, World!");
}

void setup()
{
   UNITY_BEGIN();
   RUN_TEST(test_Loger_loginfo_ln);
   UNITY_END();
}

void loop()
{
}