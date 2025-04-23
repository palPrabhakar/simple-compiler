#include "analyzer.hpp"
#include "test_utils.hpp"
#include "transformer.hpp"
#include <gtest/gtest.h>
#include <ranges>
#include <vector>

TEST(DominatorAnalyzerTest, TestDominators) {
    READ_PROGRAM("../tests/bril/dominator.json")
    BUILD_CFG()

    auto transformer = sc::CFTransformer();
    program = transformer.Transform(std::move(program));

    auto *func = program->GetFunction(0);
    auto dom = sc::DominatorAnalyzer(func);

    dom.ComputeDominance();
    std::vector<uint32_t> dom_results = {1, 3, 7, 11, 27, 35, 99, 163, 291};
    for (auto i : std::views::iota(0UL, func->GetBlockSize())) {
        EXPECT_EQ(dom.GetDominators(i).GetData()[0], dom_results[i]);
    }

    dom.ComputeImmediateDominators();
    std::vector<size_t> idom_results = {-1UL, 0, 1, 1, 3, 1, 5, 5, 5};
    for (auto i : std::views::iota(1UL, func->GetBlockSize())) {
        std::cout << dom.GetImmediateDominator(i)->GetName() << std::endl;
        EXPECT_EQ(dom.GetImmediateDominator(i),
                  func->GetBlock(idom_results[i]));
    }
}
