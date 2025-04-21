#include "index_set.hpp"
#include <gtest/gtest.h>

using namespace sc;

TEST(IndexSetTest, SetAndGetSingleBit) {
    IndexSet set(100);
    set.Set(10);
    EXPECT_TRUE(set.Get(10));
    EXPECT_FALSE(set.Get(9));
    EXPECT_FALSE(set.Get(11));
}

TEST(IndexSetTest, ResetBit) {
    IndexSet set(100);
    set.Set(20);
    EXPECT_TRUE(set.Get(20));
    set.Reset(20);
    EXPECT_FALSE(set.Get(20));
}

TEST(IndexSetTest, MultipleBits) {
    IndexSet set(100);
    set.Set(0);
    set.Set(31);
    set.Set(32);
    set.Set(63);
    set.Set(64);
    set.Set(99);

    EXPECT_TRUE(set.Get(0));
    EXPECT_TRUE(set.Get(31));
    EXPECT_TRUE(set.Get(32));
    EXPECT_TRUE(set.Get(63));
    EXPECT_TRUE(set.Get(64));
    EXPECT_TRUE(set.Get(99));

    EXPECT_FALSE(set.Get(1));
    EXPECT_FALSE(set.Get(33));
    EXPECT_FALSE(set.Get(98));
}

TEST(IndexSetTest, BoundaryConditions) {
    IndexSet set(65);
    set.Set(0);
    set.Set(64); // last valid index
    EXPECT_TRUE(set.Get(0));
    EXPECT_TRUE(set.Get(64));
}

TEST(IndexSetTest, AllBitsClearInitially) {
    IndexSet set(128);
    for (size_t i = 0; i < 128; ++i) {
        EXPECT_FALSE(set.Get(i));
    }
}

TEST(IndexSetTest, ResetWithoutSetDoesNotCrash) {
    IndexSet set(50);
    EXPECT_NO_THROW(set.Reset(10));
    EXPECT_FALSE(set.Get(10));
}

TEST(IndexSetTest, InvalidIndexAccessTriggersAssert) {
    IndexSet set(10);
#ifndef NDEBUG
    EXPECT_DEATH(set.Set(10), ".*");
    EXPECT_DEATH(set.Reset(11), ".*");
    EXPECT_DEATH(set.Get(100), ".*");
#endif
}
