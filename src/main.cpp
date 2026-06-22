// cachesim_cli — command-line entry point for the cache simulator.
//
// Usage:
//   cachesim_cli <config.json> <trace.trace>
//
// The JSON config specifies the cache hierarchy; the trace file contains
// one memory access per line. Stats are printed to stdout on completion.
//
// Example:
//   ./cachesim_cli configs/l1_l2_writeback.json traces/loop_locality.trace

#include "cachesim/MemoryHierarchy.hpp"
#include "cachesim/TraceParser.hpp"
#include "cachesim/CacheConfig.hpp"

#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;
using namespace cachesim;

// ---------------------------------------------------------------------------
// JSON -> CacheConfig conversion
// ---------------------------------------------------------------------------

static ReplacementPolicyType parseReplacementPolicy(const std::string& s) {
    if (s == "LRU")    return ReplacementPolicyType::LRU;
    if (s == "FIFO")   return ReplacementPolicyType::FIFO;
    if (s == "Random") return ReplacementPolicyType::Random;
    throw std::invalid_argument("Unknown replacement_policy: " + s);
}

static WritePolicy parseWritePolicy(const std::string& s) {
    if (s == "WriteBack")    return WritePolicy::WriteBack;
    if (s == "WriteThrough") return WritePolicy::WriteThrough;
    throw std::invalid_argument("Unknown write_policy: " + s);
}

static AllocationPolicy parseAllocationPolicy(const std::string& s) {
    if (s == "WriteAllocate")   return AllocationPolicy::WriteAllocate;
    if (s == "NoWriteAllocate") return AllocationPolicy::NoWriteAllocate;
    throw std::invalid_argument("Unknown allocation_policy: " + s);
}

static CacheConfig parseCacheConfig(const json& j) {
    CacheConfig cfg;
    cfg.cacheSizeBytes  = j.at("cache_size_bytes").get<std::uint64_t>();
    cfg.blockSizeBytes  = j.at("block_size_bytes").get<std::uint64_t>();
    cfg.associativity   = j.at("associativity").get<std::uint32_t>();

    cfg.replacementPolicy = parseReplacementPolicy(
        j.value("replacement_policy", "LRU"));
    cfg.writePolicy = parseWritePolicy(
        j.value("write_policy", "WriteBack"));
    cfg.allocationPolicy = parseAllocationPolicy(
        j.value("allocation_policy", "WriteAllocate"));

    return cfg;
}

static HierarchyConfig parseHierarchyConfig(const json& j) {
    HierarchyConfig hcfg;
    hcfg.l1Config = parseCacheConfig(j.at("l1"));

    if (j.contains("l2")) {
        hcfg.l2Config = parseCacheConfig(j.at("l2"));
    }

    hcfg.useVictimCache = j.value("victim_cache", false);

    if (j.contains("latency")) {
        const auto& lat = j.at("latency");
        hcfg.latency.l1HitCycles = lat.value("l1_hit_cycles",  4u);
        hcfg.latency.l2HitCycles = lat.value("l2_hit_cycles", 12u);
        hcfg.latency.memCycles   = lat.value("mem_cycles",   200u);
    }

    return hcfg;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: cachesim_cli <config.json> <trace.trace>\n";
        std::cerr << "Example: cachesim_cli configs/l1_l2_writeback.json traces/loop_locality.trace\n";
        return 1;
    }

    const std::string configPath = argv[1];
    const std::string tracePath  = argv[2];

    // --- Load config ---
    json configJson;
    try {
        std::ifstream f(configPath);
        if (!f.is_open()) {
            throw std::runtime_error("Cannot open config file: " + configPath);
        }
        f >> configJson;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << "\n";
        return 1;
    }

    HierarchyConfig hcfg;
    try {
        hcfg = parseHierarchyConfig(configJson);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << "\n";
        return 1;
    }

    // --- Load trace ---
    std::vector<MemoryAccess> trace;
    try {
        trace = TraceParser::parseFile(tracePath);
    } catch (const std::exception& e) {
        std::cerr << "Error loading trace: " << e.what() << "\n";
        return 1;
    }

    if (TraceParser::lastSkippedLineCount() > 0) {
        std::cerr << "Warning: " << TraceParser::lastSkippedLineCount()
                  << " malformed trace lines were skipped.\n";
    }

    std::cout << "Loaded " << trace.size() << " accesses from " << tracePath << "\n";

    // --- Run simulation ---
    MemoryHierarchy hierarchy(hcfg);
    for (const auto& access : trace) {
        hierarchy.access(access.address, access.type == AccessType::Write);
    }

    // --- Print results ---
    hierarchy.printStats();

    return 0;
}
