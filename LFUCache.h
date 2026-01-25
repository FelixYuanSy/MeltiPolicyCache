#include "ICachePolicy.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <unordered_map>

template <typename Key, typename Value>
class LruCache;
template <typename Key, typename Value>
class Freqlist {
  private:
    struct Node {
        int freq_;
        Key key_;
        Value value_;
        std::weak_ptr<Node> pre_;
        std::shared_ptr<Node> next_;

        Node() : freq_(1), next_(nullptr) {}
        Node(Key key, Value value) : freq_(1), key_(key), value_(value), next_(nullptr) {}
    };

    //定义完Node要把Node连接起来形成list
    using NodePtr = std::shared_ptr<Node>;
    //当前list的访问频率
    int freq_;
    //创建收尾节点目的：如果首尾节点挨着，说明当前频率下没有对应的节点
    NodePtr head_;
    NodePtr tail_;

  public:
    Freqlist(int freq) : freq_(freq) {
        head_ = std::make_shared<Node>();
        tail_ = std::make_shared<Node>();
        head_->next_ = tail_;
        tail_->pre_ = head_;
    }

    void addNode(NodePtr node) {
        node->pre = tail_->pre;
        node->next = tail_;
        tail_->pre.lock()->next = node; // 使用lock()获取shared_ptr
        tail_->pre = node;
    }

    void removeNode(NodePtr node) {
        auto preNode = node->pre_.lock();
        preNode->next_ = node->next_;
        node->next_->pre_ = preNode;
        node->next = nullptr;
    }
    bool isEmpty() {
        return head_->next_ == tail_;
    }

    NodePtr getFirstNode()
    {
        return head_->next_;
    }

    friend class LruCache<Key, Value>;
};
template <typename Key, typename Value>
class LruCache : public MeltiCache::ICachePolicy<Key, Value> {

  public:
    using Node = typename Freqlist<Key, Value>::Node;
    using NodePtr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    LruCache(int capacity, int maxAverageNum)
        : capacity_(capacity), minFreq_(INT8_MAX), maxAverageNum_(maxAverageNum),
          curAverageNum_(0), curTotalNum_(0) {}

    void put(Key key, Value value) override {
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end()) {
            it->second->value_ = value;
            //修改freqList里面的位置
            updateNodeFrequency(it->second);
            return;
        }
    }

  private:
    void getInternal(NodePtr node, Value &value);
    void putInternal(Key key,Value value);
    void updateNodeFrequency(NodePtr node);
    void removeFromFreqList(NodePtr node);
    void addToFreqList(NodePtr node);
    void addFreqNum();
    void decreaseFreqNum(int num);
    void handleOverMaxAverageNum();
    void updateMinFreq();
    void kickOut();


  private:
    int capacity_;                                                   //缓存总容量
    int maxAverageNum_;                                              //最大平均缓存数
    int minFreq_;                                                    //最小频数
    int curAverageNum_;                                              //当前平均频数
    int curTotalNum_;                                                //当前总缓存数
    NodeMap nodeMap_;                                                //key和节点位置映射
    std::unordered_map<int, Freqlist<Key, Value> *> freqToFreqList_; //频数和映射的当前频数freqlist
};
template <typename Key, typename Value>
void LruCache<Key, Value>::getInternal(NodePtr node,Value&value)
{
    value = node->value_;
    updateNodeFrequency(node);
}

template <typename Key, typename Value>
void LruCache<Key, Value>::putInternal(Key key,Value value)
{
    if(nodeMap_.size() >= capacity_)
    {
        kickOut();
    }
    //放入频数1list，检测minfreq是否是1，如果不是变成1且在1list里增加新node
    if(minFreq_ != 1)
    {
        minFreq_ = 1;
    }
    auto newNode = std::make_shared<Node>(key,value);
    //add to map
    nodeMap_[key] = newNode;
    freqToFreqList_.addToFreqList(newNode);
    addFreqNum();

}

template <typename Key, typename Value>
void LruCache<Key, Value>::updateNodeFrequency(NodePtr node) {
    removeFromFreqList(node);
    node->freq_++;
    addToFreqList(node);
    //判断前序频数是否变为了空，如果是空的minfreq++
    if (node->freq_ - 1 == minFreq_ && freqToFreqList_[node->freq_ - 1]->isEmpty())
    {
        minFreq_++;
    }
    //更新当前总频数和 、 当前平均频数
    addFreqNum();
}
template <typename Key, typename Value>
void LruCache<Key, Value>::addFreqNum()
{
    curTotalNum_++;
    if(nodeMap_.empty())
    {
        curAverageNum_ = 0;
    }
    else {
        curAverageNum_ = curTotalNum_ / nodeMap_.size();
        if(curAverageNum_ > maxAverageNum_)
        {
            handleOverMaxAverageNum();
        }
    }
}
template <typename Key, typename Value>
void LruCache<Key, Value>::handleOverMaxAverageNum()
{
    //用map遍历节点，找到节点后先移除list修改freq后再加入节点
    for(auto it = nodeMap_.begin();it <= nodeMap_.end();it++)
    {
        if(!it->second) continue;
        auto node = it->second;
        removeFromFreqList(node);
        node->freq_ -= maxAverageNum_ / 2;
        if(node->freq_ < 1) node->freq_ = 1;
        addToFreqList(node);

    }

    //遍历list查看更新最小频数
    updateMinFreq();
}
template <typename Key, typename Value>
void LruCache<Key, Value>::updateMinFreq()
{
    minFreq_ = INT8_MAX;
    for(auto & pair : freqToFreqList_)
    {
        if(pair.second&&!pair.second.isEmpty())
        {
            std::min(minFreq_,pair.first);
        }
    }
    if(minFreq_ == INT8_MAX) minFreq_=1;
}
template <typename Key, typename Value>
void LruCache<Key, Value>::removeFromFreqList(NodePtr node)
{
    auto preNode = node->pre_.lock();
    node->next_->pre_ = preNode;
    preNode->next_ = node->next_;
    node->next_ = nullptr;
}
template <typename Key, typename Value>
void LruCache<Key, Value>::addToFreqList(NodePtr node)
{
    if(node!=nullptr)
    {
        auto freq = freqToFreqList_.find(node->freq_);
        if(freq == freqToFreqList_.end())
        {
            freqToFreqList_[node->freq_] = new Freqlist<Key, Value>(node->freq_);
            
        }
        freqToFreqList_[node->freq_].addNode(node);
    }
}
template <typename Key, typename Value>
void LruCache<Key, Value>::kickOut()
{
    //找到最小频数head节点后一个删除
    auto node = freqToFreqList_[minFreq_].getFirstNode();
    removeFromFreqList(node);
    nodeMap_.erase(node);
    //减小平均频数
    decreaseFreqNum(node->freq_);
}
template <typename Key, typename Value>
void LruCache<Key, Value>::decreaseFreqNum(int num)
{
    curTotalNum_-=num;
    if(nodeMap_.empty())
    {
        curAverageNum_ = 0;
    }
    else {
        curAverageNum_ = curTotalNum_ / nodeMap_.size();
    }
}