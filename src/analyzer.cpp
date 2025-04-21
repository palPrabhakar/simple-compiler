#include "analyzer.hpp"
#include <ranges>
#include <unordered_set>

namespace sc {
void DominatorAnalyzer::ComputeDominance() {
    // std::unordered_set<Block *> universal(func->GetBlocks().begin(),
    //                                       func->GetBlocks().end());
    // dom[func->GetBlock(0)].insert(func->GetBlock(0));
    // for (auto i : std::views::iota(1UL, func->GetBlockSize())) {
    //     dom[func->GetBlock(i)] = universal;
    // }

    bool changed = true;
    while(changed) {
        changed = false;
        // for (auto i : std::views::iota(1UL, func->GetBlockSize())) {
        // }
    }
}
} // namespace sc
