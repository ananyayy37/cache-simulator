#include "cachesim/FIFOPolicy.hpp"

namespace cachesim {

void FIFOPolicy::initialize(std::uint64_t numSets, std::uint32_t associativity) {
    associativity_ = associativity;
    nextVictimWay_.assign(static_cast<std::size_t>(numSets), 0);
}

void FIFOPolicy::onAccess(std::uint64_t /*setIndex*/, std::uint32_t /*wayIndex*/) {
    // Intentional no-op: FIFO eviction order is unaffected by hits.
}

void FIFOPolicy::onInsert(std::uint64_t setIndex, std::uint32_t /*wayIndex*/) {
    // Advance the round-robin pointer for this set. We don't even need
    // to know *which* way was just filled - the pointer always walks
    // 0,1,2,...,A-1,0,1,... in lockstep with how CacheSet fills ways
    // during cache warm-up, and thereafter with how selectVictim() is
    // called on every subsequent eviction.
    auto& next = nextVictimWay_[setIndex];
    next = (next + 1) % associativity_;
}

std::uint32_t FIFOPolicy::selectVictim(std::uint64_t setIndex) {
    return nextVictimWay_[setIndex];
}

} // namespace cachesim
