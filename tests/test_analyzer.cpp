#include "analyzer.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <ranges>
#include <vector>

TEST(DominatorAnalyzerTest, TestDominators) {
    READ_PROGRAM("../tests/bril/dominator.json")
    BUILD_CFG()
    auto *func = program->GetFunction(0);
    auto dom = sc::DominatorAnalyzer(func);
    dom.ComputeDominance();
    std::vector<uint32_t> results = {1, 3, 7, 11, 27, 35, 99, 163, 291};
    for (auto i : std::views::iota(0UL, func->GetBlockSize())) {
        EXPECT_EQ(dom.GetDominators(i).GetData()[0], results[i]);
    }
}
