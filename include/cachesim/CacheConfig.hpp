#pragma once

#include <cstdint>
#include <string>

namespace cachesim {

// How writes are propagated to the next level (or to main memory).
enum class WritePolicy {
    WriteThrough,  // every write goes to this level AND immediately to the next level
    WriteBack      // writes only update this level; propagated later, on eviction, if dirty
};

// Whether a write that MISSES brings the block into the cache first.
enum class AllocationPolicy {
    WriteAllocate,    // miss on write -> load the block into cache, then write
    NoWriteAllocate   // miss on write -> write straight through to next level, don't cache it
};

enum class ReplacementPolicyType {
    LRU,
    FIFO,
    Random
};

// All the parameters that define one cache level.
//
// This single struct is reused for direct-mapped, set-associative, and
// fully-associative caches - the "mode" is implied by the numbers, not by
// a separate type. See README for the derivation:
//   associativity == 1                          -> direct-mapped
//   associativity == cacheSizeBytes/blockSizeBytes -> fully associative
//   anything else                                -> N-way set-associative
struct CacheConfig {
    std::uint64_t cacheSizeBytes;
    std::uint64_t blockSizeBytes;
    std::uint32_t associativity;

    ReplacementPolicyType replacementPolicy = ReplacementPolicyType::LRU;
    WritePolicy writePolicy = WritePolicy::WriteBack;
    AllocationPolicy allocationPolicy = AllocationPolicy::WriteAllocate;

    // --- Derived quantities, computed once at construction ---
    // Stored (not recomputed per-access) because address decomposition
    // happens on the hot path of every single simulated memory access -
    // we do the log2/division work once, not millions of times.
    std::uint64_t numSets = 0;
    std::uint32_t offsetBits = 0;
    std::uint32_t indexBits = 0;

    // Validates the raw inputs (sizes must be positive, powers of two,
    // associativity must evenly divide the set count) and fills in the
    // derived fields above. Throws std::invalid_argument on bad input -
    // we want construction-time failures, not silent garbage indices
    // three function calls later.
    void validateAndDerive();
};

// True if x is a power of two (and non-zero). Used to enforce that
// blockSizeBytes and numSets are powers of two, which the bit-masking
// address decomposition in Cache::decomposeAddress() depends on.
bool isPowerOfTwo(std::uint64_t x);

// log2 of a power-of-two value. Caller must guarantee x is a power of two
// (use isPowerOfTwo() first) - this function does not validate, it just
// counts trailing zero bits.
std::uint32_t log2OfPowerOfTwo(std::uint64_t x);

} // namespace cachesim
