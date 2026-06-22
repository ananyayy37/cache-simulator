#pragma once

#include "cachesim/ReplacementPolicy.hpp"
#include <vector>

namespace cachesim {

// First-In-First-Out: evicts whichever line was installed longest ago,
// regardless of how many times it's been accessed since.
//
// Implemented as a per-set "next victim" pointer that walks round-robin
// through the ways (0, 1, 2, ..., associativity-1, 0, 1, ...). This is
// simpler than tracking actual insertion timestamps: since every set
// fills its ways in order, and FIFO always evicts in that same order,
// a circular pointer is sufficient and avoids any per-way bookkeeping.
class FIFOPolicy : public ReplacementPolicy {
public:
    void initialize(std::uint64_t numSets, std::uint32_t associativity) override;

    // FIFO ignores hits entirely - a line's position in the eviction
    // order never changes just because it was read again.
    void onAccess(std::uint64_t setIndex, std::uint32_t wayIndex) override;

    void onInsert(std::uint64_t setIndex, std::uint32_t wayIndex) override;
    std::uint32_t selectVictim(std::uint64_t setIndex) override;

private:
    std::uint32_t associativity_ = 0;

    // nextVictimWay_[setIndex] = which way will be evicted next time
    // this set needs a victim. Advances by one (mod associativity)
    // every time onInsert() is called for that set.
    std::vector<std::uint32_t> nextVictimWay_;
};

} // namespace cachesim
