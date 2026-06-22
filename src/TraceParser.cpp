#include "cachesim/TraceParser.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cctype>

namespace cachesim {

std::size_t TraceParser::skippedLines_ = 0;

std::vector<MemoryAccess> TraceParser::parseFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("TraceParser: cannot open file: " + path);
    }

    skippedLines_ = 0;
    std::vector<MemoryAccess> accesses;

    std::string line;
    std::size_t lineNum = 0;

    while (std::getline(file, line)) {
        ++lineNum;

        // Strip leading whitespace.
        std::size_t start = 0;
        while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
            ++start;
        }

        // Skip blank lines and comment lines.
        if (start == line.size() || line[start] == '#') {
            continue;
        }

        char typeChar = static_cast<char>(std::toupper(
            static_cast<unsigned char>(line[start])));

        if (typeChar != 'R' && typeChar != 'W') {
            std::cerr << "TraceParser: warning: line " << lineNum
                      << " skipped (expected R or W, got '" << line[start] << "')\n";
            ++skippedLines_;
            continue;
        }

        // Skip whitespace between type and address.
        std::size_t addrStart = start + 1;
        while (addrStart < line.size() && std::isspace(
                static_cast<unsigned char>(line[addrStart]))) {
            ++addrStart;
        }

        if (addrStart == line.size()) {
            std::cerr << "TraceParser: warning: line " << lineNum
                      << " skipped (missing address)\n";
            ++skippedLines_;
            continue;
        }

        // Parse the address — accept 0x/0X prefix or bare hex.
        std::uint64_t address = 0;
        try {
            std::size_t consumed = 0;
            address = std::stoull(line.substr(addrStart), &consumed, /*base=*/16);
        } catch (const std::exception&) {
            std::cerr << "TraceParser: warning: line " << lineNum
                      << " skipped (invalid address: '" << line.substr(addrStart) << "')\n";
            ++skippedLines_;
            continue;
        }

        accesses.push_back({
            typeChar == 'W' ? AccessType::Write : AccessType::Read,
            address
        });
    }

    return accesses;
}

std::size_t TraceParser::lastSkippedLineCount() {
    return skippedLines_;
}

} // namespace cachesim
