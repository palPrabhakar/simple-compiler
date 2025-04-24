#include "analyzer.hpp"
#include "cfg.hpp"
#include <iostream>
#include <ranges>

namespace sc {
void DominatorAnalyzer::ComputeDominance() {
    dom.emplace_back(func->GetBlockSize(), false);
    dom[0].Set(0);
    for (auto _ : std::views::iota(1ul, func->GetBlockSize())) {
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
                     std::views::iota(1ul, block->GetPredecessorSize())) {
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

void DominatorAnalyzer::ComputeImmediateDominators() {
    if (dom.size() != func->GetBlockSize()) {
        ComputeDominance();
    }

    idom = std::vector<Block *>(func->GetBlockSize(), nullptr);

    for (auto i : std::views::iota(1ul, func->GetBlockSize())) {
        auto sdom = dom[i];
        sdom.Reset(i);
        auto dominators = sdom.GetDominators();
        for (auto k : dominators) {
            if (sdom == dom[k]) {
                idom[i] = func->GetBlock(k);
                break;
            }
        }
    }
}

void DominatorAnalyzer::ComputeDominanceFrontier() {
    if (idom.size() != func->GetBlockSize()) {
        ComputeImmediateDominators();
    }

    df = std::vector<IndexSet>(func->GetBlockSize(),
                               IndexSet(func->GetBlockSize()));

    // Since entry block has no predecessors
    for (auto i : std::views::iota(1ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        if (block->GetPredecessorSize() > 1) {
            for (auto k : std::views::iota(0ul, block->GetPredecessorSize())) {
                auto *runner = block->GetPredecessor(k);
                while (runner != idom[i]) {
                    df[imap[runner]].Set(i);
                    runner = idom[imap[runner]];
                }
            }
        }
    }
}

void DominatorAnalyzer::BuildIndexMap() {
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        imap[func->GetBlock(i)] = i;
    }
}

void DominatorAnalyzer::DumpDominators(std::ostream &out) {
    std::cout << "Dominators: " << func->GetName() << "\n";
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        out << block->GetName() << ":";
        for (auto k : std::views::iota(0ul, dom[imap[block]].GetSize())) {
            if (dom[imap[block]].Get(k)) {
                out << "  " << func->GetBlock(k)->GetName();
            }
        }
        out << "\n";
    }
}

void DominatorAnalyzer::DumpImmediateDominators(std::ostream &out) {
    std::cout << "Immediate Dominators: " << func->GetName() << "\n";
    auto *block = func->GetBlock(0);
    out << block->GetName() << ":";
    out << "  \n";
    for (auto i : std::views::iota(1ul, func->GetBlockSize())) {
        block = func->GetBlock(i);
        out << block->GetName() << ":";
        out << "  " << idom[i]->GetName() << "\n";
    }
}

void DominatorAnalyzer::DumpDominanceFrontier(std::ostream &out) {
    std::cout << "DominanceFrontier: " << func->GetName() << "\n";
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        out << block->GetName() << ":";
        for (auto k : std::views::iota(0ul, df[imap[block]].GetSize())) {
            if (df[imap[block]].Get(k)) {
                out << "  " << func->GetBlock(k)->GetName();
            }
        }
        out << "\n";
    }
}

} // namespace sc
