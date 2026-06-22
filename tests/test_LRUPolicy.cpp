#include <gtest/gtest.h>
#include "cachesim/LRUPolicy.hpp"

using namespace cachesim;

TEST(LRUPolicyTest, FreshlyInitializedSetEvictsWayZeroFirst) {
    LRUPolicy policy;
    policy.initialize(/*numSets=*/4, /*associativity=*/4);

    // No accesses yet - all ways tied at timestamp 0, so the scan picks
    // the first one it finds (way 0). This matches "fill empty ways in
    // order" behavior used during cache warm-up.
    EXPECT_EQ(policy.selectVictim(0), 0u);
}

TEST(LRUPolicyTest, RecentlyAccessedWayIsNotEvicted) {
    LRUPolicy policy;
    policy.initialize(1, 4);

    // Simulate filling all 4 ways.
    policy.onInsert(0, 0);
    policy.onInsert(0, 1);
    policy.onInsert(0, 2);
    policy.onInsert(0, 3);

    // Touch way 0 again - it should no longer be the LRU victim.
    policy.onAccess(0, 0);

    EXPECT_NE(policy.selectVictim(0), 0u);
}

TEST(LRUPolicyTest, EvictsTrueLeastRecentlyUsed) {
    LRUPolicy policy;
    policy.initialize(1, 3);

    policy.onInsert(0, 0);  // way 0 touched at t=1
    policy.onInsert(0, 1);  // way 1 touched at t=2
    policy.onInsert(0, 2);  // way 2 touched at t=3

    policy.onAccess(0, 0);  // way 0 touched at t=4 (now most recent)
    policy.onAccess(0, 1);  // way 1 touched at t=5

    // way 2 hasn't been touched since t=3 - it's the LRU victim now.
    EXPECT_EQ(policy.selectVictim(0), 2u);
}

TEST(LRUPolicyTest, SetsAreIndependent) {
    LRUPolicy policy;
    policy.initialize(2, 2);

    // Fill set 0 fully and keep way 1 "fresh" there.
    policy.onInsert(0, 0);
    policy.onInsert(0, 1);
    policy.onAccess(0, 1);

    // Set 1 is untouched - victim should be way 0 regardless of what
    // happened in set 0. This proves no cross-set bleed in the
    // flattened recencyCounters_ indexing.
    EXPECT_EQ(policy.selectVictim(1), 0u);
}
