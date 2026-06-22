#include "cachesim/CacheConfig.hpp"

#include <stdexcept>

namespace cachesim {

bool isPowerOfTwo(std::uint64_t x) {
    // A power of two has exactly one bit set. (x & (x-1)) clears the
    // lowest set bit, so the result is zero only when there was exactly
    // one bit to begin with. x == 0 is explicitly excluded: it has no
    // bits set, which would otherwise satisfy the formula incorrectly.
    return x != 0 && (x & (x - 1)) == 0;
}

std::uint32_t log2OfPowerOfTwo(std::uint64_t x) {
    std::uint32_t result = 0;
    while (x > 1) {
        x >>= 1;
        ++result;
    }
    return result;
}

void CacheConfig::validateAndDerive() {
    if (cacheSizeBytes == 0 || blockSizeBytes == 0 || associativity == 0) {
        throw std::invalid_argument(
            "CacheConfig: cacheSizeBytes, blockSizeBytes, and associativity must all be non-zero");
    }

    if (!isPowerOfTwo(blockSizeBytes)) {
        throw std::invalid_argument(
            "CacheConfig: blockSizeBytes must be a power of two (required for offset bit-masking)");
    }

    if (cacheSizeBytes % blockSizeBytes != 0) {
        throw std::invalid_argument(
            "CacheConfig: cacheSizeBytes must be a multiple of blockSizeBytes");
    }

    const std::uint64_t totalBlocks = cacheSizeBytes / blockSizeBytes;

    if (totalBlocks % associativity != 0) {
        throw std::invalid_argument(
            "CacheConfig: (cacheSizeBytes / blockSizeBytes) must be evenly divisible by associativity");
    }

    numSets = totalBlocks / associativity;

    if (!isPowerOfTwo(numSets)) {
        throw std::invalid_argument(
            "CacheConfig: derived number of sets must be a power of two (required for index bit-masking). "
            "Adjust cacheSizeBytes, blockSizeBytes, or associativity.");
    }

    offsetBits = log2OfPowerOfTwo(blockSizeBytes);
    indexBits = log2OfPowerOfTwo(numSets);
}

} // namespace cachesim
