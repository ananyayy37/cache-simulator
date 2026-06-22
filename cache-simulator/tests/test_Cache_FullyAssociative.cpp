#include <gtest/gtest.h>
#include "cachesim/Cache.hpp"

using namespace cachesim;

namespace {
CacheConfig fullyAssociativeConfig() {
    // 256B cache, 64B blocks, associativity == total blocks (4) -> numSets == 1
    CacheConfig cfg{256, 64, 4};
    cfg.replacementPolicy = ReplacementPolicyType::LRU;
    return cfg;
}
}

TEST(FullyAssociativeCacheTest, DerivesExactlyOneSet) {
    Cache cache(fullyAssociativeConfig());
    EXPECT_EQ(cache.config().numSets, 1u);
}

TEST(FullyAssociativeCacheTest, AnyFourDistinctBlocksCanCoexist) {
    Cache cache(fullyAssociativeConfig());

    // Fully associative means ANY addresses can coexist, regardless of
    // their numeric relationship - unlike direct-mapped/set-associative
    // where "same index" forces conflict. These four are arbitrary.
    cache.access(0x0, false);
    cache.access(0x1000, false);
    cache.access(0x999900, false);
    cache.access(0x55, false);

    EXPECT_TRUE(cache.access(0x0, false).hit);
    EXPECT_TRUE(cache.access(0x1000, false).hit);
    EXPECT_TRUE(cache.access(0x999900, false).hit);
    EXPECT_TRUE(cache.access(0x55, false).hit);
}

TEST(FullyAssociativeCacheTest, FifthBlockEvictsLRU) {
    Cache cache(fullyAssociativeConfig());

    cache.access(0x0, false);
    cache.access(0x1000, false);
    cache.access(0x2000, false);
    cache.access(0x3000, false);
    // touch all but 0x0 so it remains LRU
    cache.access(0x1000, false);
    cache.access(0x2000, false);
    cache.access(0x3000, false);

    cache.access(0x4000, false);  // 5th block -> evicts LRU (0x0)

    EXPECT_FALSE(cache.access(0x0, false).hit);
}
