#include "ICachePolicy.h"
#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

template <typename Key, typename Value> class LruCache;
template <typename Key, typename Value> class LruNode {

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

    auto it = map_.find();
    if (it != map_.end()) {
      updateExistingNode(it->second, value);
      // 调换位置到最后并且更新value, it->second为LruPtr,并且传入新value

    } else {
      // 添加节点到map和node
      addNewNode(key, value);
    }
  }

  bool get(Key key, Value &value) override {
    auto it = map_.find(key);
    if (it != map_.end()) {

      moveToMostRecent(it->second);
      value = it->second->getValue();
      return true;
    }
    return false;
  }

  Value get(Key key) override 
  {
    Value value{};
    get(key,value);
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

    auto preNode = node->prev_.lock();
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
    dummyTail_->pre_->next_ = node;
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
  std::mutex mutex;
};