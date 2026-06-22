#include "cachesim/Cache.hpp"
#include "cachesim/LRUPolicy.hpp"
#include "cachesim/FIFOPolicy.hpp"
#include "cachesim/RandomPolicy.hpp"

namespace cachesim {

std::unique_ptr<ReplacementPolicy> Cache::makePolicy(ReplacementPolicyType type) const {
    switch (type) {
        case ReplacementPolicyType::LRU:    return std::make_unique<LRUPolicy>();
        case ReplacementPolicyType::FIFO:   return std::make_unique<FIFOPolicy>();
        case ReplacementPolicyType::Random: return std::make_unique<RandomPolicy>();
    }
    // Unreachable if ReplacementPolicyType is exhaustively handled above;
    // defensive fallback to keep -Wreturn-type quiet on some compilers.
    return std::make_unique<LRUPolicy>();
}

Cache::Cache(CacheConfig config) : config_(std::move(config)) {
    config_.validateAndDerive();

    policy_ = makePolicy(config_.replacementPolicy);
    policy_->initialize(config_.numSets, config_.associativity);

    sets_.reserve(config_.numSets);
    for (std::uint64_t setIdx = 0; setIdx < config_.numSets; ++setIdx) {
        sets_.emplace_back(setIdx, config_.associativity, policy_.get());
    }
}

Cache::DecomposedAddress Cache::decomposeAddress(std::uint64_t address) const {
    // | ... tag ... | index | offset |
    //
    // offset = low offsetBits bits        -> position within the block
    // index  = next indexBits bits        -> which set
    // tag    = remaining high bits        -> identifies which block,
    //                                         among all blocks that map
    //                                         to this same set
    //
    // The masks below rely on numSets and blockSizeBytes being powers
    // of two, which CacheConfig::validateAndDerive() guarantees by
    // throwing at construction time otherwise - so we don't re-check
    // here on every single access (that would be wasted work on the
    // hot path).
    const std::uint64_t offsetMask = config_.blockSizeBytes - 1;
    const std::uint64_t indexMask = config_.numSets - 1;

    DecomposedAddress result;
    result.offset = address & offsetMask;
    result.index = (address >> config_.offsetBits) & indexMask;
    result.tag = address >> (config_.offsetBits + config_.indexBits);
    return result;
}

std::uint64_t Cache::reconstructAddress(std::uint64_t tag, std::uint64_t setIndex) const {
    // Inverse of decomposeAddress. We reconstruct the block-aligned
    // address (offset bits = 0) since write-back only needs to identify
    // WHICH block to write, not a specific byte within it.
    return (tag << (config_.offsetBits + config_.indexBits)) | (setIndex << config_.offsetBits);
}

AccessResult Cache::access(std::uint64_t address, bool isWrite) {
    if (isWrite) {
        stats_.recordWrite();
    } else {
        stats_.recordRead();
    }

    const DecomposedAddress decomposed = decomposeAddress(address);
    CacheSet& set = sets_[decomposed.index];

    const LookupResult lookup = set.find(decomposed.tag);

    if (lookup.hit) {
        stats_.recordHit();
        set.touch(lookup.wayIndex);

        if (isWrite) {
            // Write-back: just mark dirty here, the actual propagation
            // to the next level happens later, on eviction.
            // Write-through: every write must also be sent to the next
            // level immediately - MemoryHierarchy handles that
            // propagation since it's the one with access to "the next
            // level." Cache itself never reaches "upward" or "downward"
            // into other caches (see Cache class doc comment).
            if (config_.writePolicy == WritePolicy::WriteBack) {
                set.lineAt(lookup.wayIndex).setDirty(true);
            }
        }

        return AccessResult{/*hit=*/true};
    }

    // --- Miss path ---
    stats_.recordMiss();

    // No-write-allocate: a MISS on a write does not bring the block
    // into this cache at all. It still needs to be written through to
    // the next level (MemoryHierarchy's job), but this cache's storage
    // is untouched - so we stop here.
    if (isWrite && config_.allocationPolicy == AllocationPolicy::NoWriteAllocate) {
        return AccessResult{/*hit=*/false};
    }

    // Otherwise (a read miss, or a write miss under write-allocate):
    // bring the block in.
    const CacheSet::InsertResult insertResult = set.insert(decomposed.tag);

    if (isWrite && config_.writePolicy == WritePolicy::WriteBack) {
        set.lineAt(insertResult.wayIndex).setDirty(true);
    }

    AccessResult result;
    result.hit = false;

    if (insertResult.evictionOccurred && insertResult.evictedLineWasDirty) {
        // A dirty line under write-back was just evicted to make room -
        // its data must be written back to the next level. CacheSet
        // only knew the evicted TAG; Cache reconstructs the full
        // address since only Cache knows index<->set and offset width.
        result.writeBackNeeded = true;
        result.writeBackAddress = reconstructAddress(insertResult.evictedTag, decomposed.index);
    }

    return result;
}

} // namespace cachesim
