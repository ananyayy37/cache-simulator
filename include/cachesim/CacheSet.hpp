#pragma once

#include "cachesim/CacheLine.hpp"
#include "cachesim/ReplacementPolicy.hpp"
#include <optional>
#include <vector>
#include <cstdint>

namespace cachesim {

// Result of looking up a tag in a set.
struct LookupResult {
    bool hit;
    std::uint32_t wayIndex;  // valid only if hit == true
};

// One "row" of a cache: a fixed number of CacheLines (the associativity),
// plus the logic to find a tag among them or install a new one after a
// miss.
//
// CacheSet does NOT do address decomposition - by the time anything
// calls into a CacheSet, the caller (Cache) has already computed which
// set this is and what tag to look for. This keeps CacheSet testable
// completely standalone, with no addresses involved at all.
class CacheSet {
public:
    // setIndex identifies this set's position within the overall cache -
    // needed only to pass through to the shared ReplacementPolicy, which
    // tracks state per-set. `policy` is NOT owned by CacheSet (it is
    // shared across every set in the cache); the caller guarantees it
    // outlives this CacheSet.
    CacheSet(std::uint64_t setIndex, std::uint32_t associativity, ReplacementPolicy* policy);

    // Scans all ways for a line with a matching valid tag. O(associativity).
    LookupResult find(std::uint64_t tag) const;

    // Marks the line at `wayIndex` as freshly accessed (for policies
    // like LRU that care about recency). Call this after a hit.
    void touch(std::uint32_t wayIndex);

    // Installs `tag` into this set, evicting a victim if necessary.
    // Returns the way index the new line now occupies, and whether an
    // eviction actually happened (false if an empty/invalid way was
    // available, i.e. the cache wasn't full yet for this set).
    //
    // Does NOT decide what to do with evicted dirty data (e.g. write
    // back to L2) - it only reports which line was evicted and whether
    // it was dirty, leaving write-back logic to Cache, which knows
    // about write policy.
    struct InsertResult {
        std::uint32_t wayIndex;
        bool evictionOccurred;
        bool evictedLineWasDirty;
        std::uint64_t evictedTag;  // meaningful only if evictionOccurred
    };
    InsertResult insert(std::uint64_t tag);

    // Direct access to a line, e.g. so Cache can set/check the dirty bit
    // after a write hit.
    CacheLine& lineAt(std::uint32_t wayIndex) { return lines_[wayIndex]; }
    const CacheLine& lineAt(std::uint32_t wayIndex) const { return lines_[wayIndex]; }

private:
    std::uint64_t setIndex_;
    std::vector<CacheLine> lines_;
    ReplacementPolicy* policy_;  // non-owning, shared across all sets
};

} // namespace cachesim
