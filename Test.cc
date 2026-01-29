#include <iostream>
#include <string>
#include <cassert>

#include "ArcLfu.h"

using namespace std;

void testArcLfu() {
    cout << "=== Testing ArcLfu ===" << endl;

    // 测试点 1: 基础 LFU 淘汰逻辑 (频次优先)
    // 场景: 容量 2。插入 A, B。访问 A (频次变2)。插入 C。
    // 预期: B (频次1) 被淘汰。
    {
        cout << "[Test 1] Frequency Eviction..." << endl;
        ArcLfu<int, string> cache(2);
        
        cache.put(1, "A");
        cache.put(2, "B");
        
        string val;
        // 访问 1, 频次变为 2
        assert(cache.get(1, val));
        assert(val == "A");
        
        // 当前状态: {1: freq=2, 2: freq=1}
        // 插入 3, 缓存已满。最小频次是 1 (Key 2)。应淘汰 2。
        cache.put(3, "C");
        
        assert(cache.get(1, val));  // 1 应该还在
        assert(cache.get(3, val));  // 3 应该在
        assert(!cache.get(2, val)); // 2 应该被淘汰
        cout << "Passed." << endl;
    }

    // 测试点 2: 同频次淘汰逻辑 (FIFO / 先进先出)
    // 场景: 容量 2。插入 A, B。两者频次都是 1。插入 C。
    // 预期: A (最早插入) 被淘汰。
    {
        cout << "[Test 2] FIFO Tie-breaking..." << endl;
        ArcLfu<int, string> cache(2);
        
        cache.put(1, "A");
        cache.put(2, "B");
        
        // 当前状态: {1: freq=1, 2: freq=1}。1 比 2 先进入。
        // 插入 3。应淘汰 1。
        cache.put(3, "C");
        
        string val;
        assert(!cache.get(1, val)); // 1 被淘汰
        assert(cache.get(2, val));  // 2 还在
        assert(cache.get(3, val));  // 3 还在
        cout << "Passed." << endl;
    }

    cout << "All ArcLfu tests passed!" << endl;
}

int main() {
    testArcLfu();
    return 0;
}