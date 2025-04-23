#include "analyzer.hpp"
#include "cfg.hpp"
#include <iostream>
#include <ranges>

namespace sc {
void DominatorAnalyzer::ComputeDominance() {
    dom.emplace_back(func->GetBlockSize(), false);
    dom[0].Set(0);
    for (auto _ : std::views::iota(1UL, func->GetBlockSize())) {
        dom.emplace_back(func->GetBlockSize(), true);
    }

    auto rpo = GetReversePostOrder(func);

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto *block : rpo) {
            // Only entry block has no predecessor. Entry block's
            // dom set only contains itself
            if (block->GetPredecessorSize()) {
                IndexSet ns(dom[imap[block->GetPredecessor(0)]]);

                for (auto k :
                     std::views::iota(1UL, block->GetPredecessorSize())) {
                    ns = ns & dom[imap[block->GetPredecessor(k)]];
                }
                ns.Set(imap[block]);

                if (ns != dom[imap[block]]) {
                    dom[imap[block]] = ns;
                    changed = true;
                }
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
