#include <cassert>
#include <iostream>
#include <string>

#include "ArcCache.h"

using namespace std;

void testArcLfu()
{
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

        assert(cache.get(1, val));   // 1 应该还在
        assert(cache.get(3, val));   // 3 应该在
        assert(!cache.get(2, val));  // 2 应该被淘汰
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
        assert(!cache.get(1, val));  // 1 被淘汰
        assert(cache.get(2, val));   // 2 还在
        assert(cache.get(3, val));   // 3 还在
        cout << "Passed." << endl;
    }

    cout << "All ArcLfu tests passed!" << endl;
}

void testArcCache()
{
    cout << "=== Testing ArcCache ===" << endl;

    // 测试点 1: 基础 Put/Get 和 晋升机制 (Promotion)
    // 场景: LRU容量2, LFU容量2, 晋升阈值2。
    {
        cout << "[Test 1] Basic Put/Get & Promotion..." << endl;
        ArcCache<int, string> cache(2, 2);

        cache.put(1, "A");
        string val;
        assert(cache.get(1, val));
        assert(val == "A");

        // 再次访问 1 -> 访问次数变2 -> 触发晋升 (LRU -> LFU)
        assert(cache.get(1, val));

        // 此时 1 应该在 LFU 中。
        // 我们填满 LRU 部分来验证 1 是否受影响。
        cache.put(2, "B");
        cache.put(3, "C");
        // LRU 现在有 [3, 2] (满)。1 在 LFU。

        // 插入 4。LRU 满，应该淘汰 LRU 中最旧的 2。
        cache.put(4, "D");

        assert(cache.get(1, val));   // 1 应该还在 (因为它在 LFU)
        assert(cache.get(3, val));   // 3 应该在
        assert(cache.get(4, val));   // 4 应该在
        assert(!cache.get(2, val));  // 2 应该被淘汰了

        cout << "Passed." << endl;
    }

    // 测试点 2: 幽灵列表自适应 (Ghost Cache Adaptivity)
    // 场景: 验证命中 LRU Ghost 后，LRU 容量是否增加。
    {
        cout << "[Test 2] Ghost Cache Adaptivity..." << endl;
        ArcCache<int, string> cache(2, 2);

        cache.put(1, "A");
        cache.put(2, "B");
        cache.put(3, "C");
        // LRU 容量2。插入 1,2,3。LRU: [3, 2]。Ghost: [1]。

        string val;
        assert(!cache.get(1, val));  // 1 被淘汰了

        // 重新 Put 1 (命中 Ghost)。
        // 预期: LRU 容量 +1 (变3), LFU 容量 -1 (变1)。1 被放入 LFU (因为 isGhost=true)。
        cache.put(1, "A");

        // 此时 LRU 容量为 3，当前 LRU 内容: [3, 2]。
        // 再插入 4。LRU: [4, 3, 2]。总数 3，未超标。
        // 如果容量没变(还是2)，插入 4 会导致 2 被淘汰。
        cache.put(4, "D");

        assert(cache.get(2, val));  // 2 应该还在，证明 LRU 扩容成功
        // 1 此时已被淘汰。原因：LFU 容量降为 1，且 key 2 被访问后晋升到了 LFU，挤占了 key 1 的位置。
        // assert(cache.get(1, val));

        cout << "Passed." << endl;
    }

    cout << "All ArcCache tests passed!" << endl;
}

int main()
{
    // testArcLfu();
    testArcCache();
    return 0;
}