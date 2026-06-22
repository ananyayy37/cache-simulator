#pragma once

#include <cstdint>

namespace cachesim {

// The type of a single memory operation coming from a trace file or a
// synthetic workload generator.
enum class AccessType {
    Read,
    Write
};

// One memory access: "read (or write) this address."
// Deliberately a plain struct (no methods) - it is pure data, consumed by
// Cache::access() and produced by TraceParser. Keeping it dependency-free
// means TraceParser, Cache, and any future workload generator can all
// include just this one tiny header.
struct MemoryAccess {
    AccessType type;
    std::uint64_t address;
};

} // namespace cachesim
