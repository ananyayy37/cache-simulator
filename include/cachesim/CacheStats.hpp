#pragma once

#include <cstdint>

namespace cachesim {

// Pure data + derived metrics for one cache level. Deliberately has no
// logic beyond simple increments and ratio calculations - it does not
// decide WHEN to increment anything (that's Cache's job, since only
// Cache knows whether an access was a hit or miss).
class CacheStats {
public:
    void recordRead() { ++reads_; }
    void recordWrite() { ++writes_; }
    void recordHit() { ++hits_; }
    void recordMiss() { ++misses_; }

    std::uint64_t reads() const { return reads_; }
    std::uint64_t writes() const { return writes_; }
    std::uint64_t hits() const { return hits_; }
    std::uint64_t misses() const { return misses_; }
    std::uint64_t totalAccesses() const { return hits_ + misses_; }

    // Returns 0.0 if there have been no accesses yet, rather than NaN
    // from a 0/0 division - makes early/empty stats safe to print.
    double hitRate() const {
        const std::uint64_t total = totalAccesses();
        return total == 0 ? 0.0 : static_cast<double>(hits_) / static_cast<double>(total);
    }

    double missRate() const {
        return 1.0 - hitRate();
    }

private:
    std::uint64_t reads_ = 0;
    std::uint64_t writes_ = 0;
    std::uint64_t hits_ = 0;
    std::uint64_t misses_ = 0;
};

} // namespace cachesim
