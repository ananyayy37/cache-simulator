#include "cachesim/LRUPolicy.hpp"

#include <limits>

namespace cachesim {

void LRUPolicy::initialize(std::uint64_t numSets, std::uint32_t associativity) {
    associativity_ = associativity;
    // All counters start at 0. Since logicalClock_ also starts at 0 and
    // only ever increases, a freshly-initialized way (counter == 0) will
    // naturally look "oldest" and be the first victim chosen - which is
    // exactly correct for an empty/cold cache.
    recencyCounters_.assign(static_cast<std::size_t>(numSets) * associativity, 0);
}

void LRUPolicy::onAccess(std::uint64_t setIndex, std::uint32_t wayIndex) {
    ++logicalClock_;
    recencyCounters_[indexOf(setIndex, wayIndex)] = logicalClock_;
}

void LRUPolicy::onInsert(std::uint64_t setIndex, std::uint32_t wayIndex) {
    // A fresh install counts as "just used" - same bookkeeping as a hit.
    onAccess(setIndex, wayIndex);
}

std::uint32_t LRUPolicy::selectVictim(std::uint64_t setIndex) {
    std::uint32_t victimWay = 0;
    std::uint64_t oldestTimestamp = std::numeric_limits<std::uint64_t>::max();

    for (std::uint32_t way = 0; way < associativity_; ++way) {
        const std::uint64_t timestamp = recencyCounters_[indexOf(setIndex, way)];
        if (timestamp < oldestTimestamp) {
            oldestTimestamp = timestamp;
            victimWay = way;
        }
    }

    return victimWay;
}

} // namespace cachesim
