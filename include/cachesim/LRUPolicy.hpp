#pragma once

#include "cachesim/ReplacementPolicy.hpp"
#include <vector>

namespace cachesim {

// Least-Recently-Used, implemented with a per-way logical timestamp
// rather than an intrusive linked list.
//
// WHY a counter instead of a linked list:
// Classic textbook LRU uses a doubly-linked list (move-to-front on
// access, evict the tail). That's O(1) per operation but means chasing
// pointers scattered across memory on every single access.
//
// For realistic associativity (2..16 ways), this counter-based approach
// instead does: on access, write one integer (O(1)); on eviction, scan
// at most `associativity` integers to find the minimum (O(A)). Scanning
// a handful of contiguous integers is fast in practice - better cache
// locality than pointer-chasing - and the bookkeeping has far less
// surface area for bugs (no list-rewiring logic to get wrong).
//
// This is a genuine engineering tradeoff, not a shortcut: for small,
// fixed A, O(A) with great constants beats O(1) with poor ones.
class LRUPolicy : public ReplacementPolicy {
public:
    void initialize(std::uint64_t numSets, std::uint32_t associativity) override;

    void onAccess(std::uint64_t setIndex, std::uint32_t wayIndex) override;
    void onInsert(std::uint64_t setIndex, std::uint32_t wayIndex) override;
    std::uint32_t selectVictim(std::uint64_t setIndex) override;

private:
    std::uint32_t associativity_ = 0;

    // recencyCounters_[setIndex * associativity_ + wayIndex] = logical
    // timestamp of the most recent touch. Higher = more recently used.
    // Flattened into one vector (rather than vector<vector<>>) to avoid
    // per-set heap allocation overhead - one contiguous allocation total.
    std::vector<std::uint64_t> recencyCounters_;

    // Monotonically increasing global "clock". Incremented on every
    // touch across every set, so comparisons remain meaningful cache-wide.
    std::uint64_t logicalClock_ = 0;

    std::size_t indexOf(std::uint64_t setIndex, std::uint32_t wayIndex) const {
        return static_cast<std::size_t>(setIndex) * associativity_ + wayIndex;
    }
};

} // namespace cachesim
