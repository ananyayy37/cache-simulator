#include <gtest/gtest.h>
#include "cachesim/TraceParser.hpp"

#include <fstream>
#include <cstdio>

using namespace cachesim;

// Helper: write a temp trace file and return its path.
static std::string writeTempTrace(const std::string& content) {
    const std::string path = "/tmp/test_trace.trace";
    std::ofstream f(path);
    f << content;
    return path;
}

TEST(TraceParserTest, ParsesReadAndWrite) {
    auto path = writeTempTrace(
        "R 0x1000\n"
        "W 0x2000\n"
    );
    auto accesses = TraceParser::parseFile(path);

    ASSERT_EQ(accesses.size(), 2u);
    EXPECT_EQ(accesses[0].type,    AccessType::Read);
    EXPECT_EQ(accesses[0].address, 0x1000u);
    EXPECT_EQ(accesses[1].type,    AccessType::Write);
    EXPECT_EQ(accesses[1].address, 0x2000u);
}

TEST(TraceParserTest, IgnoresBlankLinesAndComments) {
    auto path = writeTempTrace(
        "# this is a comment\n"
        "\n"
        "R 0xABCD\n"
        "# another comment\n"
        "\n"
    );
    auto accesses = TraceParser::parseFile(path);

    ASSERT_EQ(accesses.size(), 1u);
    EXPECT_EQ(accesses[0].address, 0xABCDu);
}

TEST(TraceParserTest, AcceptsLowercaseRAndW) {
    auto path = writeTempTrace("r 0x10\nw 0x20\n");
    auto accesses = TraceParser::parseFile(path);

    ASSERT_EQ(accesses.size(), 2u);
    EXPECT_EQ(accesses[0].type, AccessType::Read);
    EXPECT_EQ(accesses[1].type, AccessType::Write);
}

TEST(TraceParserTest, AcceptsAddressWithoutHexPrefix) {
    auto path = writeTempTrace("R 1000\n");   // hex without 0x -> 0x1000 = 4096
    auto accesses = TraceParser::parseFile(path);

    ASSERT_EQ(accesses.size(), 1u);
    EXPECT_EQ(accesses[0].address, 0x1000u);
}

TEST(TraceParserTest, SkipsMalformedLinesAndCountsThem) {
    auto path = writeTempTrace(
        "R 0x100\n"
        "X 0x200\n"    // bad type
        "R\n"          // missing address
        "W 0x300\n"
    );
    auto accesses = TraceParser::parseFile(path);

    EXPECT_EQ(accesses.size(), 2u);
    EXPECT_EQ(TraceParser::lastSkippedLineCount(), 2u);
}

TEST(TraceParserTest, ThrowsOnMissingFile) {
    EXPECT_THROW(
        TraceParser::parseFile("/nonexistent/path/trace.trace"),
        std::runtime_error
    );
}

TEST(TraceParserTest, EmptyFileReturnsEmptyVector) {
    auto path = writeTempTrace("");
    auto accesses = TraceParser::parseFile(path);
    EXPECT_TRUE(accesses.empty());
}
