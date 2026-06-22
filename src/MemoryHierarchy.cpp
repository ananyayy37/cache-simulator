#include "cachesim/MemoryHierarchy.hpp"

#include <cassert>
#include <iostream>
#include <iomanip>

namespace cachesim {

CacheConfig MemoryHierarchy::makeVictimCacheConfig(const CacheConfig& l1cfg) const {
    // Victim cache: 8 fully-associative entries, same block size as L1.
    // Fully associative = 1 set, associativity = total blocks = 8.
    const std::uint64_t numEntries = 8;
    CacheConfig vc;
    vc.cacheSizeBytes  = numEntries * l1cfg.blockSizeBytes;
    vc.blockSizeBytes  = l1cfg.blockSizeBytes;
    vc.associativity   = static_cast<std::uint32_t>(numEntries);
    vc.replacementPolicy = ReplacementPolicyType::LRU;
    vc.writePolicy       = WritePolicy::WriteBack;
    vc.allocationPolicy  = AllocationPolicy::WriteAllocate;
    return vc;
}

MemoryHierarchy::MemoryHierarchy(HierarchyConfig config)
    : config_(std::move(config))
{
    l1_ = std::make_unique<Cache>(config_.l1Config);

    if (config_.l2Config.has_value()) {
        l2_ = std::make_unique<Cache>(config_.l2Config.value());
    }

    if (config_.useVictimCache) {
        victimCache_ = std::make_unique<Cache>(
            makeVictimCacheConfig(config_.l1Config));
    }
}

void MemoryHierarchy::access(std::uint64_t address, bool isWrite) {
    // --- L1 access ---
    const AccessResult l1Result = l1_->access(address, isWrite);

    if (l1Result.hit) {
        totalCycles_ += config_.latency.l1HitCycles;
        return;
    }

    // L1 miss — charge L1 hit time (we paid that before knowing it was a miss)
    // then continue down the hierarchy.
    totalCycles_ += config_.latency.l1HitCycles;

    // Handle dirty write-back from L1 eviction (write-back policy only).
    // This goes to the victim cache if present, otherwise directly to L2/memory.
    if (l1Result.writeBackNeeded) {
        if (victimCache_) {
            // Write the dirty evicted line into the victim cache.
            victimCache_->access(l1Result.writeBackAddress, /*isWrite=*/true);
        } else if (l2_) {
            l2_->access(l1Result.writeBackAddress, /*isWrite=*/true);
        }
        // If neither: single-level write-back just goes to "memory" (no cost
        // beyond what we already charged — simple model for now).
    }

    // --- Victim cache check (L1 miss only) ---
    if (victimCache_) {
        const AccessResult vcResult = victimCache_->access(address, isWrite);
        if (vcResult.hit) {
            // Victim cache hit: much cheaper than going to L2.
            // In real hardware the line is swapped back into L1 here
            // (that's what makes a victim cache a "victim cache" — evicted
            // lines get a second chance nearby). We model this as a fast hit.
            totalCycles_ += config_.latency.l2HitCycles / 2;  // halfway between L1 and L2
            return;
        }
    }

    // --- L2 access (if present) ---
    if (l2_) {
        const AccessResult l2Result = l2_->access(address, isWrite);

        if (l2Result.hit) {
            totalCycles_ += config_.latency.l2HitCycles;
            return;
        }

        // L2 miss — handle its write-back too.
        totalCycles_ += config_.latency.l2HitCycles;

        if (l2Result.writeBackNeeded) {
            // L2 dirty eviction goes straight to main memory (no L3 in this model).
            ++mainMemAccesses_;
            totalCycles_ += config_.latency.memCycles;
        }
    }

    // --- Main memory ---
    ++mainMemAccesses_;
    totalCycles_ += config_.latency.memCycles;
}

const CacheStats& MemoryHierarchy::l2Stats() const {
    assert(l2_ && "l2Stats() called but no L2 cache was configured");
    return l2_->stats();
}

const CacheStats& MemoryHierarchy::victimStats() const {
    assert(victimCache_ && "victimStats() called but victim cache was not enabled");
    return victimCache_->stats();
}

double MemoryHierarchy::computeAMAT() const {
    const double l1MissRate = l1_->stats().missRate();

    if (!l2_) {
        // Single-level: AMAT = L1 hit time + L1 miss rate * mem time
        return static_cast<double>(config_.latency.l1HitCycles)
             + l1MissRate * static_cast<double>(config_.latency.memCycles);
    }

    const double l2MissRate = l2_->stats().missRate();

    // Two-level: AMAT = L1 hit + L1 miss rate * (L2 hit + L2 miss rate * mem)
    return static_cast<double>(config_.latency.l1HitCycles)
         + l1MissRate * (
               static_cast<double>(config_.latency.l2HitCycles)
             + l2MissRate * static_cast<double>(config_.latency.memCycles)
           );
}

void MemoryHierarchy::printStats() const {
    auto pct = [](double r) { return r * 100.0; };

    std::cout << "\n====== Cache Simulation Results ======\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\n[L1 Cache]\n";
    std::cout << "  Reads        : " << l1_->stats().reads()   << "\n";
    std::cout << "  Writes       : " << l1_->stats().writes()  << "\n";
    std::cout << "  Hits         : " << l1_->stats().hits()    << "\n";
    std::cout << "  Misses       : " << l1_->stats().misses()  << "\n";
    std::cout << "  Hit Rate     : " << pct(l1_->stats().hitRate())  << "%\n";
    std::cout << "  Miss Rate    : " << pct(l1_->stats().missRate()) << "%\n";

    if (victimCache_) {
        std::cout << "\n[Victim Cache]\n";
        std::cout << "  Hits         : " << victimCache_->stats().hits()   << "\n";
        std::cout << "  Misses       : " << victimCache_->stats().misses() << "\n";
        std::cout << "  Hit Rate     : " << pct(victimCache_->stats().hitRate()) << "%\n";
    }

    if (l2_) {
        std::cout << "\n[L2 Cache]\n";
        std::cout << "  Reads        : " << l2_->stats().reads()   << "\n";
        std::cout << "  Writes       : " << l2_->stats().writes()  << "\n";
        std::cout << "  Hits         : " << l2_->stats().hits()    << "\n";
        std::cout << "  Misses       : " << l2_->stats().misses()  << "\n";
        std::cout << "  Hit Rate     : " << pct(l2_->stats().hitRate())  << "%\n";
        std::cout << "  Miss Rate    : " << pct(l2_->stats().missRate()) << "%\n";
    }

    std::cout << "\n[Overall]\n";
    std::cout << "  Total Cycles : " << totalCycles_       << "\n";
    std::cout << "  Main Mem I/O : " << mainMemAccesses_   << "\n";
    std::cout << "  AMAT         : " << computeAMAT()      << " cycles\n";
    std::cout << "======================================\n\n";
}

} // namespace cachesim
