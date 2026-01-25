#include <iostream>

namespace MeltiCache
{
    template <typename Key,typename Value>
    class ICachePolicy
    {
        virtual void put(Key key,Value value) = 0;
        virtual bool get(Key key,Value& value) = 0;
        virtual Value get(Key key) = 0;
    };

    
}