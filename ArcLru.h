#pragma once
#include <cstddef>
#include <memory>
#include <unordered_map>

#include "ArcNode.h"
#include "LRUCache.h"

template <typename Key, typename Value>
class ArcLru
{
  public:
    using NodeType = ArcNode<Key, Value>;
    using NodePtr = std::shared_ptr<NodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

  private:
    int capacity_;       // total cache capacity
    int ghostCapacity_;  // ghost list capacity
    int transformNeed_;  // when access number over the need ,transfer to Lfu part
    NodeMap mainCache_;
    NodeMap ghostCache_;
    NodePtr mainHead_;
    NodePtr mainTail_;
    NodePtr ghostHead_;
    NodePtr ghostTail_;

  public:
    ArcLru(int capacity, int transformNeed)
        : capacity_(capacity), transformNeed_((transformNeed)), ghostCapacity_(capacity)
    {
        initialize();
    }

    bool put(Key key, Value value)
    {
        if (capacity_ == 0) return false;
        auto it = mainCache_.find(key);
        if (it != mainCache_.end())
        {
            return updateExistingNode(it->second, value);
        }
        return addNewNode(key, value);
    }

  private:
    void initialize()
    {
        mainHead_ = std::make_shared<NodeType>();
        mainTail_ = std::make_shared<NodeType>();
        mainHead_->next_ = mainTail_;
        mainTail_->pre_ = mainHead_;

        ghostHead_ = std::make_shared<NodeType>();
        ghostTail_ = std::make_shared<NodeType>();
        ghostHead_->next_ = ghostTail_;
        ghostTail_->pre_ = ghostHead_;
    }
    bool updateExistingNode(NodePtr node, Value &value)
    {
        node->setValue(value);
        node->incrementAccessCount();
        moveToFront(node);
        return true;
    }
    bool addNewNode(Key key, Value &value)
    {
        if (mainCache_.size() >= capacity_)
        {
            evictLeastRecent();
        }
        auto newNode = std::make_shared<NodeType>(key, value);
        mainCache_[key] = newNode;
        addToFront(newNode);
    }
    void moveToFront(NodePtr node)
    {
        removeNode(node);
        addToFront(node);
    }
    void addToFront(NodePtr node)
    {
        node->pre_ = mainHead_;
        node->next_ = mainHead_->next_;
        mainHead_->next_->pre_ = node;
        mainHead_->next_ = node;
    }
    void addToGhostFront(NodePtr node)
    {
        node->pre_ = ghostHead_;
        node->next_ = ghostHead_->next_;
        ghostHead_->next_->pre_ = node;
        ghostHead_->next_ = node;
    }
    void removeNode(NodePtr node)
    {
        auto preNode = node->pre_.lock();
        preNode->next_ = node->next_;
        node->next_->pre_ = preNode;
        node->next_ = nullptr;
    }
    void removeOldestGhost()
    {
        auto lastGhostNode = ghostTail_->pre_.lock();
        removeNode(lastGhostNode);
        ghostCache_.erase(lastGhostNode->getKey());
    }
    void evictLeastRecent()
    {
        auto lastNode = mainTail_->pre_.lock();
        removeNode(lastNode);
        mainCache_.erase(lastNode->getKey());
        if (ghostCache_.size() >= ghostCapacity_)
        {
            removeOldestGhost();
        }
        addToGhostFront(lastNode);
    }
};