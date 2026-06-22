#include <gtest/gtest.h>
#include "cachesim/CacheConfig.hpp"

using namespace cachesim;

// --- isPowerOfTwo / log2OfPowerOfTwo -----------------------------------

TEST(PowerOfTwoTest, RecognizesPowersOfTwo) {
    EXPECT_TRUE(isPowerOfTwo(1));
    EXPECT_TRUE(isPowerOfTwo(2));
    EXPECT_TRUE(isPowerOfTwo(64));
    EXPECT_TRUE(isPowerOfTwo(4096));
}

TEST(PowerOfTwoTest, RejectsNonPowersOfTwoAndZero) {
    EXPECT_FALSE(isPowerOfTwo(0));
    EXPECT_FALSE(isPowerOfTwo(3));
    EXPECT_FALSE(isPowerOfTwo(100));
    EXPECT_FALSE(isPowerOfTwo(4095));
}

TEST(Log2Test, ComputesCorrectExponent) {
    EXPECT_EQ(log2OfPowerOfTwo(1), 0u);
    EXPECT_EQ(log2OfPowerOfTwo(2), 1u);
    EXPECT_EQ(log2OfPowerOfTwo(64), 6u);
    EXPECT_EQ(log2OfPowerOfTwo(4096), 12u);
}

// --- CacheConfig::validateAndDerive ------------------------------------

TEST(CacheConfigTest, DirectMapped_DerivesCorrectSetsAndBits) {
    // 32KB cache, 64B blocks, 1-way (direct-mapped)
    // totalBlocks = 32768/64 = 512, numSets = 512/1 = 512
    CacheConfig cfg{32768, 64, 1};
    cfg.validateAndDerive();

    EXPECT_EQ(cfg.numSets, 512u);
    EXPECT_EQ(cfg.offsetBits, 6u);   // log2(64)
    EXPECT_EQ(cfg.indexBits, 9u);    // log2(512)
}

TEST(CacheConfigTest, FourWaySetAssociative_DerivesCorrectSetsAndBits) {
    // 32KB cache, 64B blocks, 4-way
    // totalBlocks = 512, numSets = 512/4 = 128
    CacheConfig cfg{32768, 64, 4};
    cfg.validateAndDerive();

    EXPECT_EQ(cfg.numSets, 128u);
    EXPECT_EQ(cfg.offsetBits, 6u);
    EXPECT_EQ(cfg.indexBits, 7u);    // log2(128)
}

TEST(CacheConfigTest, FullyAssociative_DerivesSingleSet) {
    // associativity == totalBlocks -> numSets == 1, indexBits == 0
    CacheConfig cfg{4096, 64, 64};  // totalBlocks = 64, associativity = 64
    cfg.validateAndDerive();

    EXPECT_EQ(cfg.numSets, 1u);
    EXPECT_EQ(cfg.indexBits, 0u);
}

TEST(CacheConfigTest, ThrowsOnNonPowerOfTwoBlockSize) {
    CacheConfig cfg{32768, 100, 4};  // 100 is not a power of two
    EXPECT_THROW(cfg.validateAndDerive(), std::invalid_argument);
}

TEST(CacheConfigTest, ThrowsOnAssociativityNotDividingTotalBlocks) {
    // totalBlocks = 512, associativity = 5 does not divide evenly
    CacheConfig cfg{32768, 64, 5};
    EXPECT_THROW(cfg.validateAndDerive(), std::invalid_argument);
}

TEST(CacheConfigTest, ThrowsOnZeroSize) {
    CacheConfig cfg{0, 64, 4};
    EXPECT_THROW(cfg.validateAndDerive(), std::invalid_argument);
}

TEST(CacheConfigTest, ThrowsWhenDerivedSetCountNotPowerOfTwo) {
    // cacheSize/blockSize/associativity all individually fine, but
    // numSets ends up non-power-of-two: 192/64/1 = 3 sets.
    CacheConfig cfg{192, 64, 1};
    EXPECT_THROW(cfg.validateAndDerive(), std::invalid_argument);
}
