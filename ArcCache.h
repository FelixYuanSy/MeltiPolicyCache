#pragma once
#include <cstddef>
#include <memory>
#include "ArcLfu.h"
#include "ArcLru.h"
#include "LRUCache.h"

template <typename Key, typename Value>
class ArcCache : public MeltiCache::ICachePolicy<Key, Value>
{
  public:
    ArcCache(size_t capacity,size_t transformNeed):
        capacity_(capacity),transformNeed_(transformNeed),
        lru(std::unique_ptr<ArcLru<Key, Value>>(capacity,transformNeed)),
        lfu(std::unique_ptr<ArcLru<Key, Value>>(capacity))
    

    {

    }
    void put(Key key,Value value) override
    {
        //if key in the lru ghost, lfu decrease capacity, lru increase capacity
        //if key not in the lru ghost
    }

    bool get(Key key,Value value) override
    {

    }
    Value get(Key key) override
    {

    }
    private:
    size_t capacity_;
    size_t transformNeed_;
    std::unique_ptr<ArcLru<Key,  Value>> lru;
    std::unique_ptr<ArcLfu<Key,  Value>> lfu;

    private:
    bool checkGhostCaches(Key key)
    {
        //if lru ghost countain key,lfu decrease capacity, lru increase capacity
        if(lru->ghostCountain(key))
        {
            if(lfu->decreaseCapacity())
            {
                lru->increaseCapacity();
            }
        }
    }
};