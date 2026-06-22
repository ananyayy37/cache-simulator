#pragma once

#include "cachesim/MemoryAccess.hpp"
#include <string>
#include <vector>

namespace cachesim {

// Parses a plain-text memory trace file into a vector of MemoryAccess
// structs that the simulation loop can feed one-by-one into a
// MemoryHierarchy.
//
// Trace format (one access per line):
//   <R|W> <hex-address>
//
// Examples:
//   R 0x1A2B3C
//   W 0x00FF00
//   r 1000        <- lowercase and no 0x prefix are both accepted
//
// Blank lines and lines starting with '#' are silently ignored,
// so trace files can have comments.
//
// Throws std::runtime_error if the file cannot be opened.
// Malformed lines are skipped with a stderr warning rather than
// aborting — this mirrors how real simulation tools handle noisy
// trace files from hardware performance counters.
class TraceParser {
public:
    // Parses the entire file into memory and returns the access vector.
    // For very large traces (hundreds of millions of accesses) prefer
    // the streaming iterator version below, but for typical benchmark
    // traces (< a few million lines) this in-memory version is fine.
    static std::vector<MemoryAccess> parseFile(const std::string& path);

    // Returns the number of lines skipped due to parse errors during
    // the last call to parseFile(). Useful for validating trace quality.
    static std::size_t lastSkippedLineCount();

private:
    static std::size_t skippedLines_;
};

} // namespace cachesim
