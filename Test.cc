// #include "LRUCache.h"
// #include "LFUCache.h"
// #include <cassert>
// using namespace std;

// void testLru()
// {
//     std::cout<<"----Test Lru----"<<std::endl;
//     LruCache<int , std::string> cache(2);
//     cache.put(1, "value1");
//     cache.put(2, "value2");
//     std::string value;
//     cache.get(1,value);
//     assert(value == "value1");
//     cache.put(3, "value3");
//     for(int i = 1;i <=3;i++)
//     {
//         std::cout<<cache.get(i)<<std::endl;
//     }


// }



// int main()
// {
//     testLru();
//     return 0;
// }
#include <iostream>
#include <string>
#include <cassert>
#include <thread>
#include <chrono>

// 【重要】请确保你的头文件里，类名已经被改成了 class LfuCache
#include "LFUCache.h" 

using namespace std;

void test_lfu_logic() {
    cout << "=== 开始测试 LFU 核心逻辑 ===" << endl;

    // 1. 初始化容量为 2 的缓存
    // 参数：容量=2, 平均频次上限=100
    LfuCache<int, string> cache(2, 100);

    // 2. 插入 A 和 B
    cout << "[步骤] 插入 key=1(A), key=2(B)" << endl;
    cache.put(1, "A"); // 1 的频次 = 1
    cache.put(2, "B"); // 2 的频次 = 1

    // 3. 提升 A 的频次
    // 假设你还没有 public 的 get，我们通过重复 put 相同 key 来模拟增加频次
    // (根据你的逻辑，put 已存在的 key 会调用 getInternal -> updateNodeFrequency)
    cout << "[步骤] 再次访问 key=1" << endl;
    cache.put(1, "A_new"); // 1 的频次变为 2
    
    // 此时状态：
    // Key 1: freq = 2
    // Key 2: freq = 1

    // 4. 插入 C，导致缓存满，触发淘汰
    // LFU 规则：淘汰频次最低的。这里 2 的频次是 1，1 的频次是 2。
    // 所以应该淘汰 2。
    cout << "[步骤] 插入 key=3(C)，缓存已满" << endl;
    cache.put(3, "C");

    // 5. 验证结果（需要你的类提供 get 接口，或者我们通过 put 观察副作用）
    // 这里为了演示，假设你有一个简单的 public get 方法
    // 如果 key 2 被淘汰了，get(2) 应该失败或返回空
    
    // *注意*：由于我看不到你的 get 实现，这里用伪代码描述预期：
    // assert(cache.get(2) == false/empty); // 2 应该没了
    // assert(cache.get(1) == "A_new");     // 1 应该还在
    // assert(cache.get(3) == "C");         // 3 应该在

    cout << ">>> 预期结果：Key 2 (频次最低) 应该被淘汰。Key 1 和 3 应该保留。" << endl;
}

void test_same_freq_logic() {
    cout << "\n=== 测试同频次淘汰逻辑 (Tie-breaking) ===" << endl;
    LfuCache<int, string> cache(2, 100);

    // 插入 1 和 2，频次都是 1
    cache.put(1, "A"); 
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 稍微停顿确保时间差
    cache.put(2, "B");

    // 插入 3，触发淘汰
    // 1 和 2 频次都是 1。
    // 根据你的 addToFreqList 逻辑（插到链表尾部？）和 kickOut 逻辑（取链表头部？）
    // 通常 LFU 会淘汰“最早”进入该频次列表的节点。
    cache.put(3, "C");

    cout << ">>> 预期结果：如果你的链表是尾插法，getFirstNode是取头，那么 Key 1 (最早插入) 应该被淘汰。" << endl;
}

int main() {
    test_lfu_logic();
    test_same_freq_logic();
    
    cout << "\n测试结束。" << endl;
    return 0;
}