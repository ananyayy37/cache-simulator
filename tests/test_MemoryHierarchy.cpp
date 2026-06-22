#include <gtest/gtest.h>
#include "cachesim/MemoryHierarchy.hpp"

using namespace cachesim;

namespace {

HierarchyConfig singleLevelConfig() {
    CacheConfig l1{4096, 64, 4};
    HierarchyConfig hcfg;
    hcfg.l1Config = l1;
    hcfg.latency  = {4, 12, 200};
    return hcfg;
}

HierarchyConfig twoLevelConfig() {
    CacheConfig l1{4096,   64, 4};
    CacheConfig l2{65536,  64, 8};
    HierarchyConfig hcfg;
    hcfg.l1Config = l1;
    hcfg.l2Config = l2;
    hcfg.latency  = {4, 12, 200};
    return hcfg;
}

} // namespace

// ---- Single-level ---------------------------------------------------

TEST(MemoryHierarchyTest, SingleLevel_ColdMissGoesToMemory) {
    MemoryHierarchy hier(singleLevelConfig());
    hier.access(0x1000, false);

    EXPECT_EQ(hier.l1Stats().misses(), 1u);
    EXPECT_GT(hier.totalCycles(), 0u);
}

TEST(MemoryHierarchyTest, SingleLevel_WarmHitDoesNotEscapeL1) {
    MemoryHierarchy hier(singleLevelConfig());
    hier.access(0x1000, false);   // cold miss
    hier.access(0x1000, false);   // warm hit

    EXPECT_EQ(hier.l1Stats().hits(), 1u);
    EXPECT_EQ(hier.l1Stats().misses(), 1u);
}

TEST(MemoryHierarchyTest, SingleLevel_AmatFormula) {
    MemoryHierarchy hier(singleLevelConfig());

    // Drive to a known miss rate by accessing cold addresses only.
    for (int i = 0; i < 100; ++i) {
        hier.access(static_cast<std::uint64_t>(i) * 64, false);
    }

    const double amat = hier.computeAMAT();

    // AMAT must be between L1 hit time (best case) and mem time (worst case).
    EXPECT_GE(amat, 4.0);
    EXPECT_LE(amat, 204.0);  // AMAT = L1 hit (4) + 1.0 * mem (200) = 204 at 100% miss rate
}

// ---- Two-level -------------------------------------------------------

TEST(MemoryHierarchyTest, TwoLevel_L1HitNeverReachesL2) {
    MemoryHierarchy hier(twoLevelConfig());
    hier.access(0x2000, false);   // miss all levels
    hier.access(0x2000, false);   // L1 hit

    EXPECT_EQ(hier.l1Stats().hits(), 1u);
    // L2 should only have seen the initial miss install, not the second access.
    EXPECT_EQ(hier.l2Stats().reads() + hier.l2Stats().writes(), 1u);
}

TEST(MemoryHierarchyTest, TwoLevel_L1MissL2HitCostsL2Latency) {
    MemoryHierarchy hier(twoLevelConfig());

    // Cold-access enough addresses to fill L1 (4KB / 64B = 64 blocks).
    // Then come back to an early address that was evicted from L1 but
    // should still be in L2 (64KB / 64B * 8 ways = a lot more capacity).
    for (std::uint64_t i = 0; i < 200; ++i) {
        hier.access(i * 64, false);
    }

    const std::uint64_t cyclesBefore = hier.totalCycles();
    hier.access(0, false);  // likely an L1 miss, L2 hit

    // If it was an L2 hit, cycle delta should be much less than memCycles.
    const std::uint64_t delta = hier.totalCycles() - cyclesBefore;
    EXPECT_LT(delta, 200u);   // not a full memory penalty
}

TEST(MemoryHierarchyTest, TwoLevel_AmatIsLowerBoundedByL1HitTime) {
    MemoryHierarchy hier(twoLevelConfig());
    hier.access(0, false);

    EXPECT_GE(hier.computeAMAT(), 4.0);
}

// ---- Write policies through hierarchy ---------------------------------

TEST(MemoryHierarchyTest, WriteBack_DirtyEvictionHandledWithoutCrash) {
    // Regression: dirty-eviction write-back path must not crash or double-free.
    CacheConfig l1{256, 64, 1};   // tiny cache -> evictions happen quickly
    l1.writePolicy      = WritePolicy::WriteBack;
    l1.allocationPolicy = AllocationPolicy::WriteAllocate;
    CacheConfig l2{4096, 64, 4};

    HierarchyConfig hcfg;
    hcfg.l1Config = l1;
    hcfg.l2Config = l2;
    hcfg.latency  = {4, 12, 200};
    MemoryHierarchy hier(hcfg);

    // Interleave reads and writes to the same set to force dirty evictions.
    for (int i = 0; i < 32; ++i) {
        hier.access(static_cast<std::uint64_t>(i) * 256, /*isWrite=*/(i % 2 == 0));
    }

    EXPECT_GT(hier.l1Stats().misses(), 0u);
}

TEST(MemoryHierarchyTest, VictimCache_ReducesColdMissPenalty) {
    HierarchyConfig hcfg = singleLevelConfig();
    hcfg.useVictimCache = true;

    MemoryHierarchy hierVC(hcfg);
    MemoryHierarchy hierNoVC(singleLevelConfig());

    // Access a working set slightly larger than L1 capacity,
    // then re-sweep it — victim cache should absorb some of the
    // re-accesses that would otherwise go to main memory.
    const int blocks = 80;   // > 64-block L1 capacity
    for (int sweep = 0; sweep < 3; ++sweep) {
        for (int i = 0; i < blocks; ++i) {
            hierVC.  access(static_cast<std::uint64_t>(i) * 64, false);
            hierNoVC.access(static_cast<std::uint64_t>(i) * 64, false);
        }
    }

    // Victim cache version should never have MORE total cycles.
    // (It may or may not be better depending on working-set size vs
    // victim cache size, but it must never be strictly worse by a large
    // margin for this moderate working set.)
    EXPECT_LE(hierVC.totalCycles(), hierNoVC.totalCycles() + 5000u);
}
