#include <gtest/gtest.h>
#include "cachesim/CacheLine.hpp"

using namespace cachesim;

TEST(CacheLineTest, StartsInvalid) {
    CacheLine line;
    EXPECT_FALSE(line.isValid());
    EXPECT_FALSE(line.isDirty());
}

TEST(CacheLineTest, InstallSetsValidAndTagButNotDirty) {
    CacheLine line;
    line.install(0xABC);

    EXPECT_TRUE(line.isValid());
    EXPECT_FALSE(line.isDirty());
    EXPECT_EQ(line.tag(), 0xABCu);
}

TEST(CacheLineTest, SetDirtyTogglesIndependentlyOfValid) {
    CacheLine line;
    line.install(0x1);
    line.setDirty(true);

    EXPECT_TRUE(line.isValid());
    EXPECT_TRUE(line.isDirty());
}

TEST(CacheLineTest, InvalidateClearsEverything) {
    CacheLine line;
    line.install(0x42);
    line.setDirty(true);
    line.invalidate();

    EXPECT_FALSE(line.isValid());
    EXPECT_FALSE(line.isDirty());
}

TEST(CacheLineTest, ReinstallOverwritesPreviousTagAndClearsDirty) {
    CacheLine line;
    line.install(0x1);
    line.setDirty(true);

    line.install(0x2);  // simulates eviction + new block install

    EXPECT_EQ(line.tag(), 0x2u);
    EXPECT_FALSE(line.isDirty());  // fresh install should not be dirty
}
