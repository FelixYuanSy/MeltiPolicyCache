#pragma once
#include <cstddef>
#include <memory>
#include <mutex>

#include "ArcLfu.h"
#include "ArcLru.h"
#include "LRUCache.h"

template <typename Key, typename Value>
class ArcCache : public MeltiCache::ICachePolicy<Key, Value>
{
  public:
    ArcCache(size_t capacity, size_t transformNeed)
        : capacity_(capacity),
          transformNeed_(transformNeed),
          lru(std::make_unique<ArcLru<Key, Value>>(capacity, transformNeed)),
          lfu(std::make_unique<ArcLfu<Key, Value>>(capacity))

    {
    }
    void put(Key key, Value value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // if key in the lru ghost, lfu decrease capacity, lru increase capacity
        // if key not in the lru ghost
        bool isGhost = checkGhostCaches(key);
        if (lfu->countain(key) || isGhost)
        {
            lfu->put(key, value);
        }
        else
        {
            lru->put(key, value);
        }
    }

    bool get(Key key, Value& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        bool shouldTransform = false;
        if (lru->get(key, value, shouldTransform))
        {
            if (shouldTransform)
            {
                lfu->put(key, value);
                lru->remove(key);
            }
            return true;
        }
        return lfu->get(key, value);
    }
    Value get(Key key) override
    {
        Value value{};
        get(key, value);
        return value;
    }

  private:
    size_t capacity_;
    size_t transformNeed_;
    std::unique_ptr<ArcLru<Key, Value>> lru;
    std::unique_ptr<ArcLfu<Key, Value>> lfu;
    std::mutex mutex_;

  private:
    bool checkGhostCaches(Key key)
    {
        // if lru ghost countain key,lfu decrease capacity, lru increase capacity
        if (lru->ghostContain(key))
        {
            lfu->decreaseCapacity();
            lru->increaseCapacity();
            return true;
        }
        if (lfu->ghostCountain(key))
        {
            lru->decreaseCapacity();
            lfu->increaseCapacity();
            return true;
        }
        return false;
    }
};