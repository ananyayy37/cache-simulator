#pragma once

#include <cstdint>

namespace cachesim {

// Abstract interface for "which way do I evict?" decisions.
//
// Crucially, this interface operates ONLY on way indices (0..associativity-1)
// within a single set - it never sees addresses or tags. That separation is
// what lets CacheSet stay completely ignorant of *how* eviction decisions
// are made: it just calls selectVictim() and trusts the answer.
//
// A policy instance manages state for ALL sets in a cache (not just one),
// indexed by setIndex - this avoids allocating one policy object per set,
// and keeps CacheSet itself free of policy bookkeeping.
class ReplacementPolicy {
public:
    virtual ~ReplacementPolicy() = default;

    // Called every time way `wayIndex` in set `setIndex` is accessed
    // (whether the access was a hit or it's the line just installed on a
    // miss). Used by LRU to update recency; FIFO and Random ignore this.
    virtual void onAccess(std::uint64_t setIndex, std::uint32_t wayIndex) = 0;

    // Called when a NEW line is installed into way `wayIndex` of set
    // `setIndex` (i.e. right after a miss is resolved). Distinct from
    // onAccess because FIFO cares about insertion order specifically,
    // not "was this way touched."
    virtual void onInsert(std::uint64_t setIndex, std::uint32_t wayIndex) = 0;

    // Returns the way index that should be evicted next within the given
    // set, according to this policy. Does not mutate any state by itself -
    // the caller (CacheSet) is expected to call onInsert() afterward once
    // the new line has actually been installed into that way.
    virtual std::uint32_t selectVictim(std::uint64_t setIndex) = 0;

    // Called once, at cache construction, so the policy can allocate
    // per-set bookkeeping state sized to (numSets, associativity).
    virtual void initialize(std::uint64_t numSets, std::uint32_t associativity) = 0;
};

} // namespace cachesim
