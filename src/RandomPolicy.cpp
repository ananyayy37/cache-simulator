#include "cachesim/RandomPolicy.hpp"

namespace cachesim {

RandomPolicy::RandomPolicy(unsigned seed) : rng_(seed) {}

void RandomPolicy::initialize(std::uint64_t /*numSets*/, std::uint32_t associativity) {
    associativity_ = associativity;
}

void RandomPolicy::onAccess(std::uint64_t /*setIndex*/, std::uint32_t /*wayIndex*/) {
    // Intentional no-op: random eviction tracks no history.
}

void RandomPolicy::onInsert(std::uint64_t /*setIndex*/, std::uint32_t /*wayIndex*/) {
    // Intentional no-op, same reason.
}

std::uint32_t RandomPolicy::selectVictim(std::uint64_t /*setIndex*/) {
    std::uniform_int_distribution<std::uint32_t> dist(0, associativity_ - 1);
    return dist(rng_);
}

} // namespace cachesim
