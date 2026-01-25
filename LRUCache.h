#pragma once
#include "ICachePolicy.h"
#include <cmath>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <thread>

template <typename Key, typename Value>
class LruCache;
template <typename Key, typename Value>
class LruNode {

  private:
    Key key_;                    // 用来和map中协同找到Node的索引
    Value value_;                // value是所携带的内容
    size_t accessTimes_;         // 节点访问次数
    std::weak_ptr<LruNode> pre_; // 前向节点使用weak防止循环引用
    std::shared_ptr<LruNode> next_;

  public:
    LruNode(Key key, Value value) : key_(key), value_(value), accessTimes_(1) {}

    Key getKey() { return key_; }

    Value getValue() { return value_; }

    size_t getAccessCount() { return accessTimes_; }

    void setValue(const Value &value) { value_ = value; }

    void incrementAccessCount() { accessTimes_++; }

    friend class LruCache<Key, Value>; // LRUCache能够访问private里面的pre,next
};

template <typename Key, typename Value>
class LruCache : public MeltiCache::ICachePolicy<Key, Value> {
  private:
    using LruNodeType = LruNode<Key, Value>;
    // 存入指针，防止移动node时候产生大量的开销，采用指针之后，只需要移动指针
    using NodePtr = std::shared_ptr<LruNodeType>;
    using LruMap = std::unordered_map<Key, NodePtr>;

  public:
    LruCache(int capacity) : capacity_(capacity) { initializeList(); }

    void put(Key key, Value value) override {
        if (capacity_ <= 0)
            return;
        // 如果在map里找到了，更新value和把位置更新到列表最后面

        auto it = map_.find(key);
        if (it != map_.end()) {
            std::lock_guard<std::mutex> lock(mutex_);
            updateExistingNode(it->second, value);
            // 调换位置到最后并且更新value, it->second为LruPtr,并且传入新value
            return;
        }
        // 添加节点到map和node
        addNewNode(key, value);
    }

    bool get(Key key, Value &value) override {
        auto it = map_.find(key);
        if (it != map_.end()) {
            std::lock_guard<std::mutex> lock(mutex_);
            moveToMostRecent(it->second);
            value = it->second->getValue();
            return true;
        }
        return false;
    }

    Value get(Key key) override {
        Value value{};
        get(key, value);
        return value;
    }

  private:
    void initializeList() {
        dummyHead_ = std::make_shared<LruNodeType>(Key(), Value());
        // Key(),Value()写法：告诉编译器Key和Value的默认值
        dummyTail_ = std::make_shared<LruNodeType>(Key(), Value());
        dummyHead_->next_ = dummyTail_;
        dummyTail_->pre_ = dummyHead_;
    }

    void updateExistingNode(NodePtr node, Value &value) {
        node->setValue(value);
        moveToMostRecent(node);
    }

    void moveToMostRecent(NodePtr node) {
        // 删除当前位置
        removeNode(node);

        // 插入到最后端
        insertNode(node);
    }

    void removeNode(NodePtr node) {

        auto preNode = node->pre_.lock();
        if (preNode && node->next_) {
            preNode->next_ = node->next_;
            node->next_->pre_ = preNode;
            node->pre_.reset();
            node->next_ = nullptr;
        }
    }

    void insertNode(NodePtr node) {

        node->pre_ = dummyTail_->pre_.lock();
        node->next_ = dummyTail_;
        dummyTail_->pre_.lock()->next_ = node;
        dummyTail_->pre_ = node;
    }

    void addNewNode(Key &key, Value &value) {
        // 如果map长度比容量大或者等于，那么就要把最久没访问的删掉
        if (map_.size() >= capacity_) {
            evictLeastRecent();
        }
        NodePtr newnode = std::make_shared<LruNodeType>(key, value);
        insertNode(newnode);
        map_[key] = newnode;
    }

    void evictLeastRecent() {
        auto node = dummyHead_->next_;
        removeNode(node);
        map_.erase(node->getKey());
    }

  private:
    NodePtr dummyHead_;
    NodePtr dummyTail_;
    size_t capacity_; // 要创建Cache的容量
    LruMap map_;
    std::mutex mutex_;
};
template <typename Key, typename Value>
class KLruCache : LruCache<Key, Value> {
  public:
    KLruCache(int capacity, int historyCapacity, int k)
        : LruCache<Key, Value>(capacity), // 构造基类LruCache用作主缓存
          historyList_(std::make_unique<LruCache<Key, size_t>>(historyCapacity)),
          k_(k) {}

    void put(Key key, Value value) override {
        //查看主缓存里是否含有key
        bool mainMachine = LruCache<Key, Value>::get(key);
        //如果有则直接调用基类put
        if (mainMachine) {
            LruCache<Key, Value>::put(key, value);
        }
        //历史列表里更新访问次数
        size_t accessCount_ = historyList_.get(key);
        //get算访问一次，访问次数++
        accessCount_++;
        //再把更新次数的key,更新historyList
        historyList_.put(key, accessCount_);
        //map也要更新方便把到达阈值的key放入主缓存
        historyValueMap_[key] = value;

        if (accessCount_ >= k_) {
            LruCache<Key, Value>::put(key, value);
            historyList_.remove(key);
            historyValueMap_.erase(key);
        }
    }

    Value get(Key key) override {
        Value value{};
        bool mainMachine = LruCache<Key, Value>::get(key, value);

        size_t accessCount = historyList_.get(key);
        accessCount++;
        historyList_.put(key, accessCount);

        if(mainMachine) return value;

        if (accessCount >= k_) {
            auto it = historyValueMap_.find(key);
            if(it != historyValueMap_.end())
            {

              Value storedValue = it->second;
              LruCache<Key,  Value>::put(key,storedValue);
              historyList_.remove(key);
              historyValueMap_.erase(it);
              return storedValue;
            }
        }
        return value;
      
    }

  private:
    int k_; // 达到k次放入主缓存
    // 历史访问列表,Key和访问次数
    std::unique_ptr<LruCache<Key, size_t>> historyList_;
    std::unordered_map<Key, Value> historyValueMap_; // 未达到访问K次的数据未达到访问K次的数据未达到访问K次的数据
};
template<typename Key,typename Value>
class HashLruCache:public LruCache< Key,  Value>
{

    private:
    //采用vector管理分片缓存
    std::vector<std::unique_ptr<LruCache< Key,  Value>>> slicedCache_;
    int slicedNumber_;    //切片数量
    int cacheCapacity_;   //总缓存容量

    public:
    HashLruCache< Key,  Value>(int cacheCapacity,int slicedNumber):
    cacheCapacity_(cacheCapacity),
    slicedCache_(slicedNumber?slicedNumber:std::thread::hardware_concurrency())
    {
        int slicedCapacity = std::ceil(cacheCapacity/static_cast<double>(slicedNumber_));//获取每个分片应该的容量
        for(int i =0; i<slicedNumber_;i++)
        {
            slicedCache_.emplace_back(LruCache< Key,  Value>(slicedCapacity));//分片缓存存入vector进行管理
        }
    }
    void put(Key key,Value value) override
    {
        size_t slicedIndex = Hash(key)% slicedNumber_;//计算索引，放入哪一个分片中
        slicedCache_[slicedIndex]->put(key,value);
    }
    bool get(Key key,Value &value) override
    {
         size_t slicedIndex = Hash(key)% slicedNumber_;//计算索引，在哪一个分片中
         return slicedCache_[slicedIndex]->get(key,value);
    }
    Value get(Key key) override
    {
        Value value{};
        get(key,value);
        return value;

    }

    private:
    size_t Hash(Key key)
    {
        std::hash<Key> hashFunc;
        return hashFunc(key);
    }
};