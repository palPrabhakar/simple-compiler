#include "analyzer.hpp"
#include <ranges>

namespace sc {
void DominatorAnalyzer::ComputeDominance() {
    dom.emplace_back(func->GetBlockSize(), false);
    dom[0].Set(0);
    for (auto _ : std::views::iota(1UL, func->GetBlockSize())) {
        dom.emplace_back(func->GetBlockSize(), true);
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto i : std::views::iota(1UL, func->GetBlockSize())) {
            auto block = func->GetBlock(i);
            IndexSet ns(dom[imap[block->GetPredecessor(0)]]);

            for (auto k : std::views::iota(1UL, block->GetPredecessorSize())) {
                ns = ns & dom[imap[block->GetPredecessor(k)]];
            }
            ns.Set(i);

            if (ns != dom[imap[block]]) {
                dom[imap[block]] = ns;
                changed = true;
            }
        }
    }
}

void DominatorAnalyzer::BuildIndexMap() {
    for (auto i : std::views::iota(0UL, func->GetBlockSize())) {
        imap[func->GetBlock(i)] = i;
    }
}

void DominatorAnalyzer::DumpDominators(std::ostream &out) {
    for (auto i : std::views::iota(0UL, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        out << block->GetName() << "\n";
        for (auto k : std::views::iota(0UL, dom[imap[block]].GetSize())) {
            if (dom[imap[block]].Get(k)) {
                out << "  " << func->GetBlock(k)->GetName();
            }
        }
        out << "\n";
    }
}

} // namespace sc
