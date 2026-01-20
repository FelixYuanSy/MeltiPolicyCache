#include "ICachePolicy.h"
#include <list>
#include <memory>
#include <unordered_map>

template<typename Key, typename Value> class LruCache;
template<typename Key,typename Value>
class LruNode {
    
   private:
        Key key_;
        Value value_;
        size_t accessTimes_;        //节点访问次数
        std::weak_ptr<LruNode> pre_;    //前向节点使用weak防止循环引用
        std::shared_ptr<LruNode> next_; 
    
    public:
        LruNode(Key key,Value value):key_(key),value_(value),accessTimes_(1)
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
        
        friend class LruCache<Key, Value>;//LRUCache能够访问private里面的pre,next
};

template<typename Key,typename Value>
class LruCache : public MeltiCache::ICachePolicy<Key,Value>
{
    private:
    using LruNodeType = LruNode< Key,  Value>;
    using NodePtr = std::shared_ptr<LruNodeType>;   //存入指针，防止移动node时候产生大量的开销，采用指针之后，只需要移动指针
    using LruMap = std::unordered_map<Key,NodePtr>;
    size_t capacity_;   //要创建Cache的容量

    public:
    LruCache(int capacity):capacity_(capacity)
    {
        initializeList();
    }


    private:

        void initializeList()
        {
            dummyHead_ = std::make_shared<LruNodeType>(Key() ,Value()) ;//Key(),Value()写法是告诉编译器Key和Value的默认值
            dummyTail_ = std::make_shared<LruNodeType>(Key() ,Value());
            dummyHead_->next_ = dummyTail_;
            dummyTail_->pre_ = dummyHead_;

        }
    
    private:
        NodePtr dummyHead_;
        NodePtr dummyTail_;
    

        
};