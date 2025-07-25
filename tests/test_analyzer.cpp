#include "analyzers/dominator_analyzer.hpp"
#include "function.hpp"
#include "test_utils.hpp"
#include "transformers/cf_transformer.hpp"
#include "transformers/transformer.hpp"
#include <gtest/gtest.h>
#include <ranges>
#include <vector>

#define DOM_ANALYSIS()                                                         \
    program = sc::ApplyTransformation<sc::CFTransformer>(std::move(program));  \
    for (auto i : std::views::iota(0UL, program->GetSize())) {                 \
        auto *func = program->GetFunction(i);                                  \
        auto dom = sc::DominatorAnalyzer(func);                                \
        dom.ComputeDominance();                                                \
        EXPECT_EQ(func->GetBlockSize(), dom_results[i].size());                \
        for (auto k : std::views::iota(0ul, func->GetBlockSize())) {           \
            EXPECT_EQ(dom.GetDominators(k).GetData()[0], dom_results[i][k]);   \
        }                                                                      \
        dom.ComputeImmediateDominators();                                      \
        EXPECT_EQ(func->GetBlockSize(), idom_results[i].size());               \
        for (auto k : std::views::iota(1ul, func->GetBlockSize())) {           \
            EXPECT_EQ(dom.GetImmediateDominator(k),                            \
                      func->GetBlock(idom_results[i][k]));                     \
        }                                                                      \
        dom.ComputeDominanceFrontier();                                        \
        EXPECT_EQ(func->GetBlockSize(), df_results[i].size());                 \
        for (auto k : std::views::iota(0ul, func->GetBlockSize())) {           \
            EXPECT_EQ(dom.GetDominanceFrontier(k).GetData()[0],                \
                      df_results[i][k]);                                       \
        }                                                                      \
    }

TEST(DominatorAnalyzerTest, TestDominators) {
    std::vector<std::vector<uint32_t>> dom_results = {
        {1, 3, 7, 11, 27, 35, 99, 163, 291}};
    std::vector<std::vector<size_t>> idom_results = {
        {-1ul, 0, 1, 1, 3, 1, 5, 5, 5}};
    std::vector<std::vector<uint32_t>> df_results = {
        {0, 2, 8, 2, 0, 8, 256, 256, 8}};

    READ_PROGRAM("../tests/bril/dominator.json")
    BUILD_CFG()
    DOM_ANALYSIS()
}

TEST(DominatorAnalyzerTest, TestAckermann) {
    // clang-format off
    std::vector<std::vector<uint32_t>> dom_results = {
        {1, 3, 5, 13, 21, 33},
        {1}
    };
    std::vector<std::vector<size_t>> idom_results = {
        {-1ul, 0, 0, 2, 2, 0},
        {-1ul}
    };
    std::vector<std::vector<size_t>> df_results = {
        {0, 32, 32, 32, 32, 0},
        {0}
    };
    // clang-format on

    READ_PROGRAM("../tests/bril/ackermann.json")
    BUILD_CFG()
    DOM_ANALYSIS()
}

TEST(DominatorAnalyzerTest, TestRAckermann) {
    READ_PROGRAM("../tests/bril/ackermann.json")
    BUILD_CFG()

    program = sc::ApplyTransformation<sc::CFTransformer>(std::move(program));
    auto *func = program->GetFunction(0);
    auto rdom = sc::DominatorAnalyzer(func, true);

    rdom.ComputeImmediateDominators();
    for (auto k : std::views::iota(0ul, func->GetBlockSize() - 1)) {
        EXPECT_EQ(rdom.GetImmediateDominator(k), LAST_BLK(func));
    }

    rdom.ComputeDominanceFrontier();

    // entry
    auto df = rdom.GetDominanceFrontier(func->GetBlock(0));
    EXPECT_EQ(df.size(), 0);

    // exit
    df = rdom.GetDominanceFrontier(LAST_BLK(func));
    EXPECT_EQ(df.size(), 0);

    // m_zero
    df = rdom.GetDominanceFrontier(func->GetBlock(1));
    EXPECT_EQ(df.size(), 1);
    EXPECT_EQ(df[0], func->GetBlock(0));

    // m_nonzero
    df = rdom.GetDominanceFrontier(func->GetBlock(2));
    EXPECT_EQ(df.size(), 1);
    EXPECT_EQ(df[0], func->GetBlock(0));

    // n_zero
    df = rdom.GetDominanceFrontier(func->GetBlock(3));
    EXPECT_EQ(df.size(), 1);
    EXPECT_EQ(df[0], func->GetBlock(2));

    // n_nonzero
    df = rdom.GetDominanceFrontier(func->GetBlock(4));
    EXPECT_EQ(df.size(), 1);
    EXPECT_EQ(df[0], func->GetBlock(2));
}

TEST(DominatorAnalyzerTest, Test1dconv) {
    // clang-format off
    std::vector<std::vector<uint32_t>> dom_results = {
        {1, 3, 5, 9, 25, 57, 89},
        {1, 3, 7, 15, 31, 47, 67},
        {1, 3, 7, 11},
        {1}
    };
    std::vector<std::vector<size_t>> idom_results = {
        {-1ul, 0, 0, 0, 3, 4, 4},
        {-1ul, 0, 1, 2, 3, 3, 1},
        {-1ul, 0, 1, 1},
        {-1ul}
    };
    std::vector<std::vector<size_t>> df_results = {
        {0, 8, 8, 0, 16, 16, 0},
        {0, 2, 2, 10, 8, 2, 0},
        {0, 2, 2, 0},
        {0}
    };
    // clang-format on

    READ_PROGRAM("../tests/bril/1dconv.json")
    BUILD_CFG()
    DOM_ANALYSIS()
}

TEST(DominatorAnalyzerTest, TestPalindrome) {
    // clang-format off
    std::vector<std::vector<uint32_t>> dom_results = {
        {1, 3, 7, 11, 17},
        {1, 3, 7, 11, 17},
        {1, 3, 5, 13, 21, 33},
    };
    std::vector<std::vector<size_t>> idom_results = {
        {-1ul, 0, 1, 1, 0},
        {-1ul, 0, 1, 1, 0},
        {-1ul, 0, 0, 2, 2, 0},
    };
    std::vector<std::vector<size_t>> df_results = {
        {0, 18, 18, 18, 0},
        {0, 18, 18, 18, 0},
        {0, 32, 32, 32, 32, 0},
    };
    // clang-format on

    READ_PROGRAM("../tests/bril/palindrome.json")
    BUILD_CFG()
    DOM_ANALYSIS()
}
