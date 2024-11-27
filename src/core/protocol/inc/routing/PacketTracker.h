#pragma once

#include <unordered_map>
#include <list>
#include <common/inc/Definitions.h>

/**
 * @class PacketTracker
 * @brief This class is a packet tracker.
 *
 * It defines a simple packet tracker that stores a map of key-value pairs.
 * And it is used by the PacketRouter class to track packets that have already been processed.
 * The tracker is implemented as a least recently used (LRU) cache.
*/
class PacketTracker
{
private:
   // Doubly linked list to store keys in order of usage
   std::list<std::pair<uint32_t, uint32_t>> lruList;
   // Map to store key and corresponding iterator to the list
   std::unordered_map<uint32_t, std::list<std::pair<uint32_t, uint32_t>>::iterator> map;
   const uint32_t MAX_CAPACITY = 50;

   uint32_t capacity_ = MAX_CAPACITY;

   void moveToFront(uint32_t key, uint32_t value) {
      lruList.erase(map[key]);
      lruList.push_front({key, value});
      map[key] = lruList.begin();
   }

public:

   /**
    * @brief Construct a new Packet Tracker object
    *
    * @return PacketTracker
    */
   PacketTracker() {}

   /**
    * @brief Construct a new Packet Tracker object
    *
    * @param capacity the maximum number of entries to store in the tracker
    * @return PacketTracker
    */
   PacketTracker(uint32_t capacity)
   {
      capacity_ = capacity;
   }

   /**
    * @brief Get the size of the tracker
    *
    * @return the size of the tracker
    */
   inline uint32_t size()
   {
      return map.size();
   }

   /**
    * @brief Add an entry to the tracker
    *
    * @param key the key to add
    * @param value the value to add
    */
   inline void addEntry(uint32_t key, uint32_t value)
   {
      if (map.find(key) != map.end()) {
         moveToFront(key, value);
      } else {
         if (lruList.size() == capacity_) {
            auto last = lruList.back();
            map.erase(last.first);
            lruList.pop_back();
         }
         lruList.push_front({key, value});
         map[key] = lruList.begin();
      }
   }

   /**
    * @brief Remove an entry from the tracker
    *
    * @param key the key to remove
    */
   inline void removeEntry(uint32_t key)
   {
      if (map.find(key) != map.end()) {
         lruList.erase(map[key]);
         map.erase(key);
      }
   }

   /**
    * @brief Clear the tracker
    */
   inline void clearMap()
   {
      lruList.clear();
      map.clear();
   }

   /**
    * @brief Check if a key exists in the tracker
    *
    * @param key the key to check
    * @return true if the key exists, false otherwise
    */
   inline bool keyExists(uint32_t key)
   {
      return map.find(key) != map.end();
   }

   /**
    * @brief Find a value in the tracker or return a default value
    *
    * @param key the key to find
    * @param defaultValue the default value to return if the key is not found
    * @return V the value found or the default value
    */
   template <typename K, typename V>
   inline V findOrDefault(const K &key, const V &defaultValue)
   {
      auto it = map.find(key);
      if (it != map.end()) {
         moveToFront(key, it->second->second);
         return it->second->second;
      }
      else {
         return defaultValue;
      }
   }
};
