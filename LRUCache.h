#include "ICachePolicy.h"
#include <list>
#include <memory>
#include <unordered_map>


template<typename Key,typename Value>
class LRUNode {
    
   private:
        Key key_;
        Value value_;
        size_t accessTimes_;        //节点访问次数
        std::weak_ptr<LRUNode> pre_;    //前向节点使用weak防止循环引用
        std::shared_ptr<LRUNode> next_; 
    
    public:
        LRUNode(Key key,Value value):key_(key),value_(value),accessTimes_(1)
        {}
        
        Key getKey()
        {
            return key_;
        }

        Value getValue()
        {
            return value_;
        }

        size_t getAccessCount()
        {
            return accessTimes_;
        }

        void setValue(const Value &value)
        {
            value_=value;
        }

        void incrementAccessCount()
        {
            accessTimes_++;
        }
};

template<typename Key,typename Value>
class LruCache : public MeltiCache::ICachePolicy<Key,Value>
{
    
};