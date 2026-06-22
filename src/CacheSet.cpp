#include "cachesim/CacheSet.hpp"

namespace cachesim {

CacheSet::CacheSet(std::uint64_t setIndex, std::uint32_t associativity, ReplacementPolicy* policy)
    : setIndex_(setIndex), lines_(associativity), policy_(policy) {}

LookupResult CacheSet::find(std::uint64_t tag) const {
    for (std::uint32_t way = 0; way < lines_.size(); ++way) {
        if (lines_[way].isValid() && lines_[way].tag() == tag) {
            return LookupResult{true, way};
        }
    }
    return LookupResult{false, 0};
}

void CacheSet::touch(std::uint32_t wayIndex) {
    policy_->onAccess(setIndex_, wayIndex);
}

CacheSet::InsertResult CacheSet::insert(std::uint64_t tag) {
    // First, look for an empty (invalid) way - this is the "cache isn't
    // full yet for this set" case, common during warm-up. We fill empty
    // ways in order (0, 1, 2, ...) which is the assumption FIFOPolicy's
    // round-robin pointer depends on (see FIFOPolicy.hpp comment).
    for (std::uint32_t way = 0; way < lines_.size(); ++way) {
        if (!lines_[way].isValid()) {
            lines_[way].install(tag);
            policy_->onInsert(setIndex_, way);
            return InsertResult{way, /*evictionOccurred=*/false, false, 0};
        }
    }

    // Set is full - ask the policy which way to evict.
    const std::uint32_t victimWay = policy_->selectVictim(setIndex_);
    const bool wasDirty = lines_[victimWay].isDirty();
    const std::uint64_t evictedTag = lines_[victimWay].tag();

    lines_[victimWay].install(tag);
    policy_->onInsert(setIndex_, victimWay);

    return InsertResult{victimWay, /*evictionOccurred=*/true, wasDirty, evictedTag};
}

} // namespace cachesim
