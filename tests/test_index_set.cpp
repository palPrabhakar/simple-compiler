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
    EXPECT_DEATH(set.Set(10), ".*");
    EXPECT_DEATH(set.Reset(11), ".*");
    EXPECT_DEATH(set.Get(100), ".*");
}

TEST(IndexSetTest, TestIntersection) {
    IndexSet set1(64); // 64-bit set
    IndexSet set2(64); // 64-bit set

    set1.Set(5);
    set1.Set(10);
    set2.Set(10);
    set2.Set(20);

    IndexSet result = set1 & set2;

    EXPECT_FALSE(result.Get(5));
    EXPECT_TRUE(result.Get(10));
    EXPECT_FALSE(result.Get(20));
}

TEST(IndexSetTest, TestUnionDifferentSizes) {
    IndexSet set1(64);  // 64-bit set
    IndexSet set2(128); // 128-bit set

    set1.Set(5);
    set2.Set(5);
    set2.Set(64);

    IndexSet result = set1 | set2;

    EXPECT_TRUE(result.Get(5));
    EXPECT_TRUE(result.Get(64));
    EXPECT_FALSE(result.Get(100));
}

TEST(IndexSetTest, TestIntersectionDifferentSizes) {
    IndexSet set1(64);  // 64-bit set
    IndexSet set2(128); // 128-bit set

    set1.Set(5);
    set2.Set(5);
    set2.Set(64);

    IndexSet result = set1 & set2;

    EXPECT_TRUE(result.Get(5));
    EXPECT_DEATH(result.Get(64), ".*"); // Only 64 in set2, not in set1
}

TEST(IndexSetTest, TestUnionEmptySet) {
    IndexSet set1(64); // 64-bit set
    IndexSet set2(64); // 64-bit set

    IndexSet result = set1 | set2;

    for (size_t i = 0; i < 64; ++i) {
        EXPECT_FALSE(result.Get(i)); // Should remain empty
    }
}

TEST(IndexSetTest, TestIntersectionEmptySet) {
    IndexSet set1(64); // 64-bit set
    IndexSet set2(64); // 64-bit set

    IndexSet result = set1 & set2;

    for (size_t i = 0; i < 64; ++i) {
        EXPECT_FALSE(result.Get(i)); // Should remain empty
    }
}

TEST(IndexSetTest, TestUnionWithSelf) {
    IndexSet set1(64); // 64-bit set
    set1.Set(5);
    set1.Set(10);

    IndexSet result = set1 | set1;

    EXPECT_TRUE(result.Get(5));
    EXPECT_TRUE(result.Get(10));
    EXPECT_FALSE(result.Get(20));
}

TEST(IndexSetTest, TestIntersectionWithSelf) {
    IndexSet set1(64); // 64-bit set
    set1.Set(5);
    set1.Set(10);

    IndexSet result = set1 & set1;

    EXPECT_TRUE(result.Get(5));
    EXPECT_TRUE(result.Get(10));
    EXPECT_FALSE(result.Get(20));
}

TEST(IndexSetTest, TestUnionWithNoCommonElements) {
    IndexSet set1(64);  // 64-bit set
    IndexSet set2(64);  // 64-bit set

    set1.Set(5);
    set1.Set(10);
    set2.Set(15);
    set2.Set(20);

    IndexSet result = set1 | set2;

    EXPECT_TRUE(result.Get(5));
    EXPECT_TRUE(result.Get(10));
    EXPECT_TRUE(result.Get(15));
    EXPECT_TRUE(result.Get(20));
    EXPECT_FALSE(result.Get(25));  // Not set in either set
}

TEST(IndexSetTest, TestIntersectionWithNoCommonElements) {
    IndexSet set1(64);  // 64-bit set
    IndexSet set2(64);  // 64-bit set

    set1.Set(5);
    set1.Set(10);
    set2.Set(15);
    set2.Set(20);

    IndexSet result = set1 & set2;

    EXPECT_FALSE(result.Get(5));   // Not common
    EXPECT_FALSE(result.Get(10));  // Not common
    EXPECT_FALSE(result.Get(15));  // Not common
    EXPECT_FALSE(result.Get(20));  // Not common
}

TEST(IndexSetTest, TestUnionWithAllElementsInCommon) {
    IndexSet set1(64);  // 64-bit set
    IndexSet set2(64);  // 64-bit set

    set1.Set(5);
    set1.Set(10);
    set2.Set(5);
    set2.Set(10);

    IndexSet result = set1 | set2;

    EXPECT_TRUE(result.Get(5));  // Common element
    EXPECT_TRUE(result.Get(10)); // Common element
    EXPECT_FALSE(result.Get(20)); // Not set in either set
}

TEST(IndexSetTest, TestIntersectionWithAllElementsInCommon) {
    IndexSet set1(64);  // 64-bit set
    IndexSet set2(64);  // 64-bit set

    set1.Set(5);
    set1.Set(10);
    set2.Set(5);
    set2.Set(10);

    IndexSet result = set1 & set2;

    EXPECT_TRUE(result.Get(5));  // Common element
    EXPECT_TRUE(result.Get(10)); // Common element
    EXPECT_FALSE(result.Get(20)); // Not set in either set
}

TEST(IndexSetTest, TestUnionWithSingleElement) {
    IndexSet set1(64);  // 64-bit set
    IndexSet set2(64);  // 64-bit set

    set1.Set(5);
    set2.Set(10);

    IndexSet result = set1 | set2;

    EXPECT_TRUE(result.Get(5));
    EXPECT_TRUE(result.Get(10));
    EXPECT_FALSE(result.Get(15));
}

TEST(IndexSetTest, TestIntersectionWithSingleElement) {
    IndexSet set1(64);  // 64-bit set
    IndexSet set2(64);  // 64-bit set

    set1.Set(5);
    set2.Set(5);

    IndexSet result = set1 & set2;

    EXPECT_TRUE(result.Get(5));
    EXPECT_FALSE(result.Get(10));
}

TEST(IndexSetTest, TestUnionWithLargeSetSize) {
    IndexSet set1(128);  // 128-bit set
    IndexSet set2(128);  // 128-bit set

    set1.Set(5);
    set1.Set(64);
    set2.Set(64);
    set2.Set(100);

    IndexSet result = set1 | set2;

    EXPECT_TRUE(result.Get(5));   // From set1
    EXPECT_TRUE(result.Get(64));  // Common element
    EXPECT_TRUE(result.Get(100)); // From set2
    EXPECT_FALSE(result.Get(120)); // Not set in either set
}

TEST(IndexSetTest, TestIntersectionWithLargeSetSize) {
    IndexSet set1(128);  // 128-bit set
    IndexSet set2(128);  // 128-bit set

    set1.Set(5);
    set1.Set(64);
    set2.Set(64);
    set2.Set(100);

    IndexSet result = set1 & set2;

    EXPECT_TRUE(result.Get(64));  // Common element
    EXPECT_FALSE(result.Get(5));  // Not in set2
    EXPECT_FALSE(result.Get(100)); // Not in set1
}

TEST(IndexSetTest, TestUnionWithOverlappingAndNonOverlappingSets) {
    IndexSet set1(128);  // 128-bit set
    IndexSet set2(128);  // 128-bit set

    set1.Set(5);
    set1.Set(64);
    set2.Set(10);
    set2.Set(64);

    IndexSet result = set1 | set2;

    EXPECT_TRUE(result.Get(5));   // From set1
    EXPECT_TRUE(result.Get(10));  // From set2
    EXPECT_TRUE(result.Get(64));  // Common element
    EXPECT_FALSE(result.Get(100)); // Not set in either set
}

TEST(IndexSetTest, TestIntersectionWithOverlappingAndNonOverlappingSets) {
    IndexSet set1(128);  // 128-bit set
    IndexSet set2(128);  // 128-bit set

    set1.Set(5);
    set1.Set(64);
    set2.Set(10);
    set2.Set(64);

    IndexSet result = set1 & set2;

    EXPECT_FALSE(result.Get(5));   // Not common
    EXPECT_FALSE(result.Get(10));  // Not common
    EXPECT_TRUE(result.Get(64));   // Common element
    EXPECT_FALSE(result.Get(100)); // Not set in either set
}

TEST(IndexSetTest, TestUnionWithMaxSize) {
    IndexSet set1(1000);  // 1000-bit set
    IndexSet set2(1000);  // 1000-bit set

    for (size_t i = 0; i < 500; ++i) {
        set1.Set(i);
    }
    for (size_t i = 300; i < 800; ++i) {
        set2.Set(i);
    }

    IndexSet result = set1 | set2;

    for (size_t i = 0; i < 300; ++i) {
        EXPECT_TRUE(result.Get(i)); // From set1
    }
    for (size_t i = 300; i < 800; ++i) {
        EXPECT_TRUE(result.Get(i)); // From set2
    }
    for (size_t i = 800; i < 1000; ++i) {
        EXPECT_FALSE(result.Get(i)); // Not set in either
    }
}

TEST(IndexSetTest, TestIntersectionWithMaxSize) {
    IndexSet set1(1000);  // 1000-bit set
    IndexSet set2(1000);  // 1000-bit set

    for (size_t i = 0; i < 500; ++i) {
        set1.Set(i);
    }
    for (size_t i = 300; i < 800; ++i) {
        set2.Set(i);
    }

    IndexSet result = set1 & set2;

    for (size_t i = 300; i < 500; ++i) {
        EXPECT_TRUE(result.Get(i)); // Common elements
    }
    for (size_t i = 500; i < 800; ++i) {
        EXPECT_FALSE(result.Get(i)); // Set2 elements not in set1
    }
    for (size_t i = 800; i < 1000; ++i) {
        EXPECT_FALSE(result.Get(i)); // Not set in either
    }
}
