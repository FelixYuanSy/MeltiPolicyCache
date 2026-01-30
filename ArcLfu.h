#pragma once
#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "ArcNode.h"
template <typename Key, typename Value>
class ArcLfu
{
  public:
    using NodeType = ArcNode<Key, Value>;
    using NodePtr = std::shared_ptr<NodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;
    using FreqMap = std::unordered_map<size_t, std::list<NodePtr>>;  // Key is the frequncy of Node, Value is the Node's
                                                                     // frequncy which can match the Key

  public:
    ArcLfu(size_t capacity) : mainCapacity_(capacity), ghostCapacity_(capacity), minFreq_(0) { initializeLists(); }

    bool put(Key key, Value value)
    {
        if (mainCapacity_ == 0) return false;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = mainCache_.find(key);
        if (it != mainCache_.end())
        {
            return updateExistingNode(it->second, value);
        }
        return addNewNode(key, value);
    }

    bool get(Key key, Value& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = mainCache_.find(key);
        if (it != mainCache_.end())
        {
            updateNodeFrequency(it->second);
            value = it->second->getValue();
            return true;
        }
        return false;
    }
    
    void decreaseCapacity()
    {
        if(mainCache_.size()==mainCapacity_)
        {
            evictLeastFrequent();
        }
        --mainCapacity_;
    }
    void increaseCapacity()
    {
        ++mainCapacity_;
    }
    bool ghostCountain(Key key)
    {
        auto it = ghostCache_.find(key);
        return it != ghostCache_.end();
    }
    bool countain(Key key)
    {
       return mainCache_.find(key) != mainCache_.end();
    }

  private:
    size_t mainCapacity_;       // main cache total capacity
    size_t ghostCapacity_;  // ghost cache capacity
    size_t minFreq_;        // minimal of the node frequency
    NodeMap mainCache_;
    NodeMap ghostCache_;
    FreqMap freqMap_;

    NodePtr ghostHead_;
    NodePtr ghostTail_;
    std::mutex mutex_;

  private:
    void initializeLists()
    {
        ghostHead_ = std::make_shared<NodeType>();
        ghostTail_ = std::make_shared<NodeType>();
        ghostHead_->next_ = ghostTail_;
        ghostTail_->pre_ = ghostHead_;
    }

    bool updateExistingNode(NodePtr node, Value value)
    {
        node->setValue(value);
        updateNodeFrequency(node);
        return true;
    }

    bool addNewNode(Key key, Value value)
    {
        if (mainCache_.size() >= mainCapacity_)
        {
            evictLeastFrequent();
        }
        auto newNode = std::make_shared<NodeType>(key, value);
        mainCache_[key] = newNode;
        freqMap_[1].push_back(newNode);
        minFreq_ = 1;
        return true;
    }
    void updateNodeFrequency(NodePtr node)
    {
        auto oldFreq = node->getAccessCount();
        node->incrementAccessCount();
        auto newFreq = node->getAccessCount();
        freqMap_[oldFreq].remove(node);
        if (freqMap_[oldFreq].empty())
        {
            freqMap_.erase(oldFreq);
            if (minFreq_ == oldFreq)
            {
                minFreq_++;
            }
        }
        freqMap_[newFreq].push_back(node);
    }

    // 从 Ghost 链表中移除节点（双向链表操作）
    void removeNode(NodePtr node)
    {
        auto preNode = node->pre_.lock();
        if (preNode)
        {
            preNode->next_ = node->next_;
            if (node->next_)
            {
                node->next_->pre_ = preNode;
            }
        }
        node->next_ = nullptr;
    }

    void evictLeastFrequent()
    {
        auto it = freqMap_.find(minFreq_);
        if (it == freqMap_.end() || it->second.empty()) return;

        // LFU 策略：移除 minFreq 链表头部的节点（最早进入该频率的节点）
        NodePtr victim = it->second.front();
        it->second.pop_front();
        if (it->second.empty())
        {
            freqMap_.erase(it);
        }
        mainCache_.erase(victim->getKey());

        // 将淘汰的节点加入 Ghost 缓存 (用于 ARC 策略调整)
        if (ghostCache_.size() >= ghostCapacity_)
        {
            // 移除 Ghost 链表中最旧的 (Tail 的前一个)
            auto lastGhost = ghostTail_->pre_.lock();
            removeNode(lastGhost);
            ghostCache_.erase(lastGhost->getKey());
        }
        // 加入 Ghost 链表头部
        victim->pre_ = ghostHead_;
        victim->next_ = ghostHead_->next_;
        ghostHead_->next_->pre_ = victim;
        ghostHead_->next_ = victim;
        ghostCache_[victim->getKey()] = victim;
    }
};