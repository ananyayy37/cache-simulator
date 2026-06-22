#include <gtest/gtest.h>
#include "cachesim/RandomPolicy.hpp"

using namespace cachesim;

TEST(RandomPolicyTest, AlwaysReturnsValidWayIndex) {
    RandomPolicy policy(/*seed=*/123);
    policy.initialize(4, 8);

    for (int i = 0; i < 1000; ++i) {
        std::uint32_t victim = policy.selectVictim(0);
        EXPECT_LT(victim, 8u);
    }
}

TEST(RandomPolicyTest, SameSeedProducesSameSequence) {
    RandomPolicy policyA(/*seed=*/7);
    RandomPolicy policyB(/*seed=*/7);
    policyA.initialize(1, 16);
    policyB.initialize(1, 16);

    // Reproducibility check - same seed must give identical eviction
    // sequences, which matters for trustworthy, re-runnable benchmarks.
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(policyA.selectVictim(0), policyB.selectVictim(0));
    }
}

TEST(RandomPolicyTest, AssociativityOneAlwaysReturnsWayZero) {
    RandomPolicy policy(1);
    policy.initialize(1, 1);

    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(policy.selectVictim(0), 0u);
    }
}
