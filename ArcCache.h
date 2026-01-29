#pragma once
#include "ArcLfu.h"
#include "ArcLru.h"
#include "LRUCache.h"

template <typename Key, typename Value>
class ArcCache : public MeltiCache::ICachePolicy<Key, Value>
{
  private:
    size_t capacity_;
    size_t transformThreshold_;  // 转换所需数量
    ArcLfu<Key, Value> lfu;
    ArcLru<Key, Value> lru;

  public:
    ArcCache(size_t capacity, size_t transformThreshold)
        : capacity_(capacity),
          transformThreshold_(transformThreshold),
          lfu(0, capacity),  // LFU 初始容量为 0，但 Ghost 容量需为总容量
          lru(capacity, transformThreshold, capacity)
    {
    }

    void put(Key key, Value value) override
    {
        Value temp;
        // 1. Check LFU
        if (lfu.get(key, temp))
        {
            lfu.put(key, value);
            return;
        }

        // 2. Check LRU
        bool needTransform = false;
        if (lru.get(key, temp, needTransform))
        {
            if (needTransform)
            {
                lru.remove(key);
                lfu.increaseCapacity();
                lru.decreaseCapacity();
                lfu.put(key, value);
            }
            else
            {
                lru.put(key, value);
            }
            return;
        }

        // 3. Check Ghosts (Adaptation)
        if (lru.checkGhostCaches(key, temp))
        {
            lru.removeFromGhost(key);
            lru.increaseCapacity();
            lfu.decreaseCapacity();
            lru.put(key, value);
            return;
        }
        if (lfu.checkGhostCaches(key, temp))
        {
            lfu.removeFromGhost(key);
            lfu.increaseCapacity();
            lru.decreaseCapacity();
            lfu.put(key, value);
            return;
        }

        // 4. New Item
        lru.put(key, value);
    }

    bool get(Key key, Value &value) override
    {
        if (lfu.get(key, value)) return true;

        bool needTransform = false;
        if (lru.get(key, value, needTransform))
        {
            if (needTransform)
            {
                lru.remove(key);
                lfu.increaseCapacity();
                lru.decreaseCapacity();
                lfu.put(key, value);
            }
            return true;
        }
        return false;
    }

    Value get(Key key) override
    {
        Value value{};
        get(key, value);
        return value;
    }
};