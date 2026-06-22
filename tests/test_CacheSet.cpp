#include <gtest/gtest.h>
#include "cachesim/CacheSet.hpp"
#include "cachesim/LRUPolicy.hpp"
#include "cachesim/FIFOPolicy.hpp"

using namespace cachesim;

TEST(CacheSetTest, FindOnEmptySetIsAlwaysMiss) {
    LRUPolicy policy;
    policy.initialize(1, 4);
    CacheSet set(0, 4, &policy);

    EXPECT_FALSE(set.find(0xABC).hit);
}

TEST(CacheSetTest, InsertThenFindHits) {
    LRUPolicy policy;
    policy.initialize(1, 4);
    CacheSet set(0, 4, &policy);

    set.insert(0x10);
    auto result = set.find(0x10);

    EXPECT_TRUE(result.hit);
}

TEST(CacheSetTest, DifferentTagIsMiss) {
    LRUPolicy policy;
    policy.initialize(1, 4);
    CacheSet set(0, 4, &policy);

    set.insert(0x10);
    EXPECT_FALSE(set.find(0x20).hit);
}

TEST(CacheSetTest, FillsEmptyWaysBeforeEvicting) {
    LRUPolicy policy;
    policy.initialize(1, 2);
    CacheSet set(0, 2, &policy);

    auto r1 = set.insert(0x1);
    auto r2 = set.insert(0x2);

    EXPECT_FALSE(r1.evictionOccurred);
    EXPECT_FALSE(r2.evictionOccurred);
    EXPECT_NE(r1.wayIndex, r2.wayIndex);  // distinct ways, not overwriting
}

TEST(CacheSetTest, EvictsWhenSetIsFull) {
    LRUPolicy policy;
    policy.initialize(1, 2);
    CacheSet set(0, 2, &policy);

    set.insert(0x1);
    set.insert(0x2);
    auto r3 = set.insert(0x3);  // set is now full, must evict

    EXPECT_TRUE(r3.evictionOccurred);
    // Original tags 0x1 and 0x2 - one of them must now be gone.
    EXPECT_TRUE(r3.evictedTag == 0x1 || r3.evictedTag == 0x2);
}

TEST(CacheSetTest, EvictedCleanLineReportsNotDirty) {
    LRUPolicy policy;
    policy.initialize(1, 1);
    CacheSet set(0, 1, &policy);

    set.insert(0x1);  // single way, never marked dirty
    auto r2 = set.insert(0x2);

    EXPECT_TRUE(r2.evictionOccurred);
    EXPECT_FALSE(r2.evictedLineWasDirty);
}

TEST(CacheSetTest, EvictedDirtyLineReportsDirty) {
    LRUPolicy policy;
    policy.initialize(1, 1);
    CacheSet set(0, 1, &policy);

    auto r1 = set.insert(0x1);
    set.lineAt(r1.wayIndex).setDirty(true);

    auto r2 = set.insert(0x2);

    EXPECT_TRUE(r2.evictionOccurred);
    EXPECT_TRUE(r2.evictedLineWasDirty);
}

TEST(CacheSetTest, TouchUpdatesLRURecency) {
    LRUPolicy policy;
    policy.initialize(1, 2);
    CacheSet set(0, 2, &policy);

    set.insert(0x1);  // way 0
    set.insert(0x2);  // way 1

    set.touch(0);     // way 0 is now most-recently-used

    auto r3 = set.insert(0x3);  // must evict the OTHER way (way 1)

    EXPECT_EQ(r3.evictedTag, 0x2u);
}

TEST(CacheSetTest, FIFORespectsInsertionOrderNotTouchOrder) {
    FIFOPolicy policy;
    policy.initialize(1, 2);
    CacheSet set(0, 2, &policy);

    set.insert(0x1);  // inserted first
    set.insert(0x2);  // inserted second

    set.touch(0);     // FIFO ignores this - should NOT save way 0

    auto r3 = set.insert(0x3);

    EXPECT_EQ(r3.evictedTag, 0x1u);  // first one in is still first one out
}
