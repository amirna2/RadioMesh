#include <unity.h>
#include "PacketTracker.h"

void test_PacketTracker_addEntry(void)
{
   PacketTracker tracker;
   TEST_ASSERT_EQUAL(0, tracker.size());
   tracker.addEntry(1, 2);
   tracker.addEntry(2, 3);
   tracker.addEntry(3, 4);
   tracker.addEntry(4, 5);
   tracker.addEntry(5, 6);
   tracker.addEntry(6, 7);
   tracker.addEntry(7, 8);
   tracker.addEntry(8, 9);
   TEST_ASSERT_EQUAL(8, tracker.size());
   TEST_ASSERT_EQUAL(true, tracker.keyExists(1));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(2));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(3));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(4));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(5));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(6));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(7));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(8));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(9));
}

void test_PacketTracker_addEntry_evicts_lru(void)
{
   PacketTracker tracker(3);
   TEST_ASSERT_EQUAL(0, tracker.size());
   tracker.addEntry(1, 2);
   tracker.addEntry(2, 3);
   tracker.addEntry(3, 4);
   TEST_ASSERT_EQUAL(3, tracker.size());
   TEST_ASSERT_EQUAL(true, tracker.keyExists(1));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(2));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(3));
   tracker.addEntry(4, 5); // This should evict the least recently used entry (key 1)
   TEST_ASSERT_EQUAL(3, tracker.size());
   TEST_ASSERT_EQUAL(false, tracker.keyExists(1));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(4));
}

void test_PacketTracker_removeEntry(void)
{
   PacketTracker tracker(3);
   tracker.addEntry(1, 2);
   tracker.addEntry(2, 3);
   tracker.addEntry(3, 4);
   TEST_ASSERT_EQUAL(3, tracker.size());
   tracker.removeEntry(2);
   TEST_ASSERT_EQUAL(2, tracker.size());
   TEST_ASSERT_EQUAL(false, tracker.keyExists(2));
}
void test_PacketTracker_clearMap(void)
{
   PacketTracker tracker;
   tracker.addEntry(1, 2);
   tracker.addEntry(2, 3);
   tracker.addEntry(3, 4);
   tracker.addEntry(4, 5);
   tracker.addEntry(5, 6);
   tracker.addEntry(6, 7);
   tracker.clearMap();
   TEST_ASSERT_EQUAL(0, tracker.size());
   TEST_ASSERT_EQUAL(false, tracker.keyExists(1));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(2));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(3));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(4));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(5));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(6));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(7));
}

void test_PacketTracker_keyExists(void)
{
   PacketTracker tracker;
   tracker.addEntry(1, 2);
   tracker.addEntry(2, 3);
   tracker.addEntry(3, 4);
   tracker.addEntry(4, 5);
   tracker.addEntry(5, 6);
   tracker.addEntry(6, 7);
   TEST_ASSERT_EQUAL(true, tracker.keyExists(1));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(2));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(3));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(4));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(5));
   TEST_ASSERT_EQUAL(true, tracker.keyExists(6));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(7));
   TEST_ASSERT_EQUAL(false, tracker.keyExists(8));
}

void test_PacketTracker_findOrDefault(void)
{
   PacketTracker tracker(3);
   tracker.addEntry(1, 2);
   tracker.addEntry(2, 3);
   tracker.addEntry(3, 4);
   TEST_ASSERT_EQUAL(2, tracker.findOrDefault(1, 0));
   TEST_ASSERT_EQUAL(3, tracker.findOrDefault(2, 0));
   TEST_ASSERT_EQUAL(4, tracker.findOrDefault(3, 0));
   TEST_ASSERT_EQUAL(0, tracker.findOrDefault(4, 0));
}

void setup()
{
   UNITY_BEGIN();
   RUN_TEST(test_PacketTracker_addEntry);
   RUN_TEST(test_PacketTracker_clearMap);
   RUN_TEST(test_PacketTracker_keyExists);
   RUN_TEST(test_PacketTracker_findOrDefault);
   RUN_TEST(test_PacketTracker_removeEntry);
   RUN_TEST(test_PacketTracker_addEntry_evicts_lru);
   UNITY_END();
}

void loop()
{
}
