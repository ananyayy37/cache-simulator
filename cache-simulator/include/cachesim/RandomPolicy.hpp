#pragma once

#include "cachesim/ReplacementPolicy.hpp"
#include <random>

namespace cachesim {

// Evicts a uniformly random way on every eviction. No per-way state
// needed at all - the simplest possible policy, useful as a baseline
// to compare LRU/FIFO against ("how much does a smart policy actually
// help over picking randomly?").
//
// Seeded explicitly (rather than std::random_device every time) so
// simulation runs are REPRODUCIBLE - this matters when you're reporting
// benchmark numbers and need to be able to re-run and get the same
// result, not a different one each time.
class RandomPolicy : public ReplacementPolicy {
public:
    explicit RandomPolicy(unsigned seed = 42);

    void initialize(std::uint64_t numSets, std::uint32_t associativity) override;

    // Random eviction has no concept of access history to track.
    void onAccess(std::uint64_t setIndex, std::uint32_t wayIndex) override;
    void onInsert(std::uint64_t setIndex, std::uint32_t wayIndex) override;

    std::uint32_t selectVictim(std::uint64_t setIndex) override;

private:
    std::uint32_t associativity_ = 0;
    std::mt19937 rng_;
};

} // namespace cachesim
