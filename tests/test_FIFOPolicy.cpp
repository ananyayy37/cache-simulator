#include <gtest/gtest.h>
#include "cachesim/FIFOPolicy.hpp"

using namespace cachesim;

TEST(FIFOPolicyTest, EvictsInInsertionOrder) {
    FIFOPolicy policy;
    policy.initialize(1, 3);

    // Fill all 3 ways in order.
    policy.onInsert(0, 0);
    policy.onInsert(0, 1);
    policy.onInsert(0, 2);

    // First one inserted (way 0) should be the first evicted.
    EXPECT_EQ(policy.selectVictim(0), 0u);
}

TEST(FIFOPolicyTest, HitsDoNotAffectEvictionOrder) {
    FIFOPolicy policy;
    policy.initialize(1, 2);

    policy.onInsert(0, 0);
    policy.onInsert(0, 1);

    // Repeatedly "hit" way 0 - under LRU this would save it, but FIFO
    // must ignore this entirely.
    policy.onAccess(0, 0);
    policy.onAccess(0, 0);
    policy.onAccess(0, 0);

    EXPECT_EQ(policy.selectVictim(0), 0u);  // still way 0, insertion order unchanged
}

TEST(FIFOPolicyTest, AfterEvictionPointerAdvancesRoundRobin) {
    FIFOPolicy policy;
    policy.initialize(1, 2);

    policy.onInsert(0, 0);  // victim pointer -> way 1
    policy.onInsert(0, 1);  // victim pointer -> way 0 (wraps around)

    EXPECT_EQ(policy.selectVictim(0), 0u);

    // Simulate evicting way 0 and installing a new line there.
    policy.onInsert(0, 0);  // victim pointer -> way 1

    EXPECT_EQ(policy.selectVictim(0), 1u);
}
