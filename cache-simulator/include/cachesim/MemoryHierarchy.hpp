#pragma once

#include "cachesim/Cache.hpp"
#include "cachesim/CacheStats.hpp"
#include <memory>
#include <optional>

namespace cachesim {

// Latency model for each level of the hierarchy (in cycles).
// Deliberately simple — we're simulating memory hierarchy behaviour,
// not a full microarchitecture. These numbers match typical modern
// values and can be overridden via JSON config.
struct LatencyModel {
    std::uint32_t l1HitCycles  = 4;
    std::uint32_t l2HitCycles  = 12;
    std::uint32_t memCycles    = 200;
};

// Configuration for the full hierarchy — passed in by main.cpp / tests.
struct HierarchyConfig {
    CacheConfig l1Config;
    std::optional<CacheConfig> l2Config;   // absent = single-level cache
    bool useVictimCache = false;           // small FA buffer between L1 and L2
    LatencyModel latency;
};

// Wires L1, optional L2, optional victim cache, and "main memory" together.
// On every access it:
//   1. Tries L1.
//   2. On L1 miss (with victim cache): checks the victim cache first.
//   3. On L1 miss (without victim / victim also misses): tries L2.
//   4. On L2 miss (or no L2): charges main-memory latency.
//   5. Handles dirty write-backs from each level to the next.
//
// AMAT (Average Memory Access Time) is derived from hit/miss stats and
// the latency model after the simulation completes.
class MemoryHierarchy {
public:
    explicit MemoryHierarchy(HierarchyConfig config);

    // Service one memory access through the full hierarchy.
    void access(std::uint64_t address, bool isWrite);

    // --- Statistics accessors ---
    const CacheStats& l1Stats()      const { return l1_->stats(); }
    const CacheStats& l2Stats()      const;   // asserts L2 exists
    const CacheStats& victimStats()  const;   // asserts victim cache exists

    std::uint64_t totalCycles()      const { return totalCycles_; }

    // AMAT = L1 hit time + L1 miss rate * (L2 hit time + L2 miss rate * mem time)
    // (or simplified if L2 is absent)
    double computeAMAT() const;

    void printStats() const;

private:
    HierarchyConfig config_;

    std::unique_ptr<Cache> l1_;
    std::unique_ptr<Cache> l2_;          // nullptr if single-level
    std::unique_ptr<Cache> victimCache_; // nullptr if not enabled

    std::uint64_t totalCycles_ = 0;
    std::uint64_t mainMemAccesses_ = 0;

    // Victim cache config: 8 fully-associative lines, same block size as L1.
    // Size: 8 * blockSize bytes. Associativity = 8 (fully associative).
    // This matches the classic HP PA-7200 victim cache design.
    CacheConfig makeVictimCacheConfig(const CacheConfig& l1cfg) const;
};

} // namespace cachesim
