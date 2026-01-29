#pragma once
#include <cstddef>
#include <list>
#include <memory>
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
    ArcLfu(size_t capacity):capacity_(capacity),ghostCapacity_(capacity),minFreq_(0)
    {
        initializeLists();
    }

    void put(Key key,Value value)
    {
        
    }

  private:
    size_t capacity_;  // main cache total capacity
    size_t ghostCapacity_; //ghost cache capacity
    size_t minFreq_;   //minimal of the node frequency
    NodeMap mainCache_;
    NodeMap ghostCache_;
    FreqMap freqMap_;

    NodePtr ghostHead_;
    NodePtr ghostTail_;

    private:
    void initializeLists()
    {
        ghostHead_ = std::make_shared<NodeType>();
        ghostTail_ = std::make_shared<NodeType>();
        ghostHead_->next_ = ghostTail_;
        ghostTail_->pre_ = ghostHead_;
    }
};