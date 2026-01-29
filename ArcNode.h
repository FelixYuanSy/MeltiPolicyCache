#pragma once
#include <cstddef>
#include <memory>
template <typename Key, typename Value>
class ArcNode
{
  private:
    Key key_;
    Value value_;
    size_t accessCount_;  // 访问次数
    std::shared_ptr<ArcNode> next_;
    std::weak_ptr<ArcNode> pre_;

  public:
    ArcNode() : accessCount_(1), next_(nullptr) {}
    ArcNode(Key key, Value value) : accessCount_(1), key_(key), value_(value), next_(nullptr) {}
    void setValue(Value value) { value_ = value; }
    Value getValue() { return value_; }
    Key getKey() { return key_; }
    size_t getAccessCount() { return accessCount_; }
    void incrementAccessCount() { accessCount_++; }

    template <typename K, typename V>
    friend class ArcLru;
    template <typename K, typename V>
    friend class ArcLfu;
};