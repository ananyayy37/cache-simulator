#include <gtest/gtest.h>
#include "cachesim/Cache.hpp"

using namespace cachesim;

namespace {
CacheConfig directMappedConfig() {
    // 1KB cache, 64B blocks, 1-way -> 16 sets, direct-mapped
    CacheConfig cfg{1024, 64, 1};
    cfg.replacementPolicy = ReplacementPolicyType::LRU;
    return cfg;
}
}

TEST(DirectMappedCacheTest, FirstAccessIsAlwaysMiss) {
    Cache cache(directMappedConfig());
    auto result = cache.access(0x0, /*isWrite=*/false);
    EXPECT_FALSE(result.hit);
}

TEST(DirectMappedCacheTest, RepeatedAccessToSameAddressHits) {
    Cache cache(directMappedConfig());
    cache.access(0x100, false);
    auto result = cache.access(0x100, false);
    EXPECT_TRUE(result.hit);
}

TEST(DirectMappedCacheTest, TwoAddressesMappingToSameSetEvictEachOther) {
    // 16 sets, 64B blocks -> set index uses bits [6:9], so addresses
    // 1024 apart (0x400) map to the SAME set but different tags.
    Cache cache(directMappedConfig());

    cache.access(0x0, false);     // loads into set 0
    cache.access(0x400, false);   // same set, different tag -> evicts the first

    auto result = cache.access(0x0, false);  // must miss again - it was evicted
    EXPECT_FALSE(result.hit);
}

TEST(DirectMappedCacheTest, StatsTrackHitsAndMissesCorrectly) {
    Cache cache(directMappedConfig());

    cache.access(0x0, false);   // miss
    cache.access(0x0, false);   // hit
    cache.access(0x0, false);   // hit
    cache.access(0x400, false); // miss (evicts 0x0's line)

    EXPECT_EQ(cache.stats().hits(), 2u);
    EXPECT_EQ(cache.stats().misses(), 2u);
    EXPECT_DOUBLE_EQ(cache.stats().hitRate(), 0.5);
}

TEST(DirectMappedCacheTest, ReadsAndWritesCountedSeparately) {
    Cache cache(directMappedConfig());

    cache.access(0x0, false);  // read
    cache.access(0x40, true);  // write (different address)

    EXPECT_EQ(cache.stats().reads(), 1u);
    EXPECT_EQ(cache.stats().writes(), 1u);
}
