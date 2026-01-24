#include "ICachePolicy.h"
#include <memory>

template<typename Key ,typename Value >
class Freqlist
{
    private:
    struct Node
    {
        int freq_;
        Key key_;
        Value value_;
        std::weak_ptr<Node> pre_;
        std::shared_ptr<Node> next_;

        Node():freq_(1),next_(nullptr){}
        Node(Key key,Value value):freq_(1),key_(key),value_(value),next_(nullptr){}

    };
};