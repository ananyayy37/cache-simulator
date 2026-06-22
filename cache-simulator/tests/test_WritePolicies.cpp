#include <gtest/gtest.h>
#include "cachesim/Cache.hpp"

using namespace cachesim;

namespace {
CacheConfig baseConfig() {
    // 256B cache, 64B blocks, 1-way -> 4 sets, direct-mapped.
    // Small and direct-mapped so it's easy to force evictions
    // deterministically in tests.
    CacheConfig cfg{256, 64, 1};
    cfg.replacementPolicy = ReplacementPolicyType::LRU;
    return cfg;
}
}

// --- Write-Allocate vs No-Write-Allocate --------------------------------

TEST(WritePolicyTest, WriteAllocate_WriteMissBringsBlockIntoCache) {
    CacheConfig cfg = baseConfig();
    cfg.allocationPolicy = AllocationPolicy::WriteAllocate;
    Cache cache(cfg);

    cache.access(0x0, /*isWrite=*/true);   // miss, but should install the block

    auto result = cache.access(0x0, false); // subsequent read
    EXPECT_TRUE(result.hit);
}

TEST(WritePolicyTest, NoWriteAllocate_WriteMissDoesNotInstallBlock) {
    CacheConfig cfg = baseConfig();
    cfg.allocationPolicy = AllocationPolicy::NoWriteAllocate;
    Cache cache(cfg);

    cache.access(0x0, /*isWrite=*/true);   // miss, should NOT install

    auto result = cache.access(0x0, false); // subsequent read
    EXPECT_FALSE(result.hit);  // still a miss - block was never cached
}

// --- Write-Back vs Write-Through ----------------------------------------

TEST(WritePolicyTest, WriteBack_DirtyEvictionReportsWriteBackNeeded) {
    CacheConfig cfg = baseConfig();
    cfg.writePolicy = WritePolicy::WriteBack;
    cfg.allocationPolicy = AllocationPolicy::WriteAllocate;
    Cache cache(cfg);

    cache.access(0x0, /*isWrite=*/true);   // installs + marks dirty (write miss, allocate)

    // 0x100 maps to the SAME set (4 sets, 64B blocks -> set index from
    // bits [6:7]; 0x100 = 256 decimal, same set as 0x0 in a 4-set cache).
    auto result = cache.access(0x100, /*isWrite=*/false);  // evicts the dirty line

    EXPECT_TRUE(result.writeBackNeeded);
    EXPECT_EQ(result.writeBackAddress, 0x0u);
}

TEST(WritePolicyTest, WriteBack_CleanEvictionDoesNotReportWriteBack) {
    CacheConfig cfg = baseConfig();
    cfg.writePolicy = WritePolicy::WriteBack;
    Cache cache(cfg);

    cache.access(0x0, /*isWrite=*/false);  // read miss - installs, but NOT dirty
    auto result = cache.access(0x100, false);  // evicts the clean line

    EXPECT_FALSE(result.writeBackNeeded);
}

TEST(WritePolicyTest, WriteThrough_NeverMarksLinesDirtyEvenOnWriteHit) {
    CacheConfig cfg = baseConfig();
    cfg.writePolicy = WritePolicy::WriteThrough;
    Cache cache(cfg);

    cache.access(0x0, /*isWrite=*/true);   // write miss, installs
    cache.access(0x0, /*isWrite=*/true);   // write HIT this time

    // Force an eviction of that same line - under write-through it
    // should never have been marked dirty, so no write-back is needed
    // (write-through already propagated each write immediately, by
    // definition of the policy - MemoryHierarchy handles that
    // propagation; Cache itself just shouldn't report a write-back).
    auto result = cache.access(0x100, false);

    EXPECT_FALSE(result.writeBackNeeded);
}
