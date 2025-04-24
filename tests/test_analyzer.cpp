#include "analyzer.hpp"
#include "test_utils.hpp"
#include "transformer.hpp"
#include <gtest/gtest.h>
#include <ranges>
#include <vector>

#define DOM_ANALYSIS()                                                         \
    auto transformer = sc::CFTransformer();                                    \
    program = transformer.Transform(std::move(program));                       \
    for (auto i : std::views::iota(0UL, program->GetSize())) {                 \
        auto *func = program->GetFunction(i);                                  \
        auto dom = sc::DominatorAnalyzer(func);                                \
        dom.ComputeDominance();                                                \
        EXPECT_EQ(func->GetBlockSize(), dom_results[i].size());                \
        for (auto k : std::views::iota(0UL, func->GetBlockSize())) {           \
            EXPECT_EQ(dom.GetDominators(k).GetData()[0], dom_results[i][k]);   \
        }                                                                      \
        dom.ComputeImmediateDominators();                                      \
        EXPECT_EQ(func->GetBlockSize(), idom_results[i].size());               \
        for (auto k : std::views::iota(1UL, func->GetBlockSize())) {           \
            EXPECT_EQ(dom.GetImmediateDominator(k),                            \
                      func->GetBlock(idom_results[i][k]));                     \
        }                                                                      \
    }

TEST(DominatorAnalyzerTest, TestDominators) {
    std::vector<std::vector<uint32_t>> dom_results = {
        {1, 3, 7, 11, 27, 35, 99, 163, 291}};
    std::vector<std::vector<size_t>> idom_results = {
        {-1UL, 0, 1, 1, 3, 1, 5, 5, 5}};

    READ_PROGRAM("../tests/bril/dominator.json")
    BUILD_CFG()
    DOM_ANALYSIS()
}

TEST(DominatorAnalyzerTest, TestAckermann) {
    std::vector<std::vector<uint32_t>> dom_results = {{1, 3, 5, 13, 21, 33},
                                                      {1}};
    std::vector<std::vector<size_t>> idom_results = {{-1UL, 0, 0, 2, 2, 0},
                                                     {-1UL}};

    READ_PROGRAM("../tests/bril/ackermann.json")
    BUILD_CFG()
    DOM_ANALYSIS()
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
    // clang-format on

    READ_PROGRAM("../tests/bril/1dconv.json")
    BUILD_CFG()
    DOM_ANALYSIS()
}
