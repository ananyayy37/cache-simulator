#pragma once

#include "cachesim/CacheConfig.hpp"
#include "cachesim/CacheSet.hpp"
#include "cachesim/CacheStats.hpp"
#include "cachesim/ReplacementPolicy.hpp"
#include "cachesim/MemoryAccess.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace cachesim {

// Result of one access into this cache - tells the caller (e.g.
// MemoryHierarchy, or Cache itself when acting as an L2) whether it hit,
// and if it missed AND the eviction was dirty under write-back, what
// address needs to be written to the next level down.
struct AccessResult {
    bool hit;

    // Only meaningful when hit == false AND a dirty eviction occurred
    // under WriteBack policy: the next level down needs this address
    // written to it before (or as part of) servicing the new request.
    bool writeBackNeeded = false;
    std::uint64_t writeBackAddress = 0;
};

// A single level of cache: direct-mapped, set-associative, or fully
// associative, determined entirely by the numbers in CacheConfig (see
// CacheConfig.hpp for the derivation). Owns its own sets, its own
// replacement policy instance, and its own stats - it has NO knowledge
// of any other cache level. Hierarchy wiring (L1 calling into L2 on a
// miss) is MemoryHierarchy's job, not this class's.
class Cache {
public:
    explicit Cache(CacheConfig config);

    // Services one memory access. `isWrite` distinguishes a load from a
    // store, which matters for write-policy branching (write-through vs
    // write-back, write-allocate vs no-write-allocate) and for stats.
    AccessResult access(std::uint64_t address, bool isWrite);

    const CacheStats& stats() const { return stats_; }
    const CacheConfig& config() const { return config_; }

    // Address decomposition - exposed publicly (not just used
    // internally) so it can be unit-tested directly and so other code
    // (e.g. a future visualization dashboard) can reuse it without
    // duplicating the bit-masking logic.
    struct DecomposedAddress {
        std::uint64_t tag;
        std::uint64_t index;
        std::uint64_t offset;
    };
    DecomposedAddress decomposeAddress(std::uint64_t address) const;

    // Reconstructs the original block-aligned address from a tag and
    // set index - the inverse of decomposeAddress, needed when reporting
    // which address must be written back to the next level on a dirty
    // eviction (CacheSet only knows the evicted TAG, not the full
    // address - only Cache can re-derive the address, since only Cache
    // knows the index<->set mapping and the offset bit width).
    std::uint64_t reconstructAddress(std::uint64_t tag, std::uint64_t setIndex) const;

private:
    CacheConfig config_;
    std::unique_ptr<ReplacementPolicy> policy_;
    std::vector<CacheSet> sets_;
    CacheStats stats_;

    std::unique_ptr<ReplacementPolicy> makePolicy(ReplacementPolicyType type) const;
};

} // namespace cachesim
