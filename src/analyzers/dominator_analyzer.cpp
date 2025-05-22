#include "analyzers/dominator_analyzer.hpp"
#include "analyzers/cfg.hpp"
#include <ranges>

namespace sc {
// DominatorAnalyzer begin
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
                IndexSet ns(dom[block->GetPredecessor(0)->GetIndex()]);

                for (auto k :
                     std::views::iota(1ul, block->GetPredecessorSize())) {
                    ns = ns & dom[block->GetPredecessor(k)->GetIndex()];
                }
                ns.Set(block->GetIndex());

                if (ns != dom[block->GetIndex()]) {
                    dom[block->GetIndex()] = ns;
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
        auto dominators = sdom.GetBlocks();
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
            for (auto *runner : block->GetPredecessors()) {
                while (runner != idom[i]) {
                    df[runner->GetIndex()].Set(i);
                    runner = idom[runner->GetIndex()];
                }
            }
        }
    }
}

void DominatorAnalyzer::BuildDominatorTree() {
    if (idom.size() != func->GetBlockSize()) {
        ComputeImmediateDominators();
    }

    dtree = std::vector<std::vector<Block *>>(func->GetBlockSize());
    for (auto i : std::views::iota(1ul, idom.size())) {
        dtree[idom[i]->GetIndex()].push_back(func->GetBlock(i));
    }
}

void DominatorAnalyzer::BuildRPODominatorTree() {
    // successor's are in reverse postorder
    if (idom.size() != func->GetBlockSize()) {
        ComputeImmediateDominators();
    }

    dtree = std::vector<std::vector<Block *>>(func->GetBlockSize());
    auto rpo = GetReversePostOrder(func);
    for (auto *blk : rpo) {
        if (blk == func->GetBlock(0)) {
            continue;
        }

        dtree[idom[blk->GetIndex()]->GetIndex()].push_back(blk);
    }
}

std::vector<Block *>
DominatorAnalyzer::GetDominanceFrontier(Block *block) const {
    auto blocks = df.at(block->GetIndex()).GetBlocks();
    std::vector<Block *> result;
    for (auto i : blocks) {
        result.push_back(func->GetBlock(i));
    }
    return result;
}

void DominatorAnalyzer::DumpDominators(std::ostream &out) const {
    std::cout << "Dominators: " << func->GetName() << "\n";
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        out << block->GetName() << ":";
        for (auto k :
             std::views::iota(0ul, dom.at(block->GetIndex()).GetSize())) {
            if (dom.at(block->GetIndex()).Get(k)) {
                out << "  " << func->GetBlock(k)->GetName();
            }
        }
        out << "\n";
    }
}

void DominatorAnalyzer::DumpImmediateDominators(std::ostream &out) const {
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

void DominatorAnalyzer::DumpDominanceFrontier(std::ostream &out) const {
    std::cout << "DominanceFrontier: " << func->GetName() << "\n";
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        out << "  " << block->GetName() << ":";
        for (auto k :
             std::views::iota(0ul, df.at(block->GetIndex()).GetSize())) {
            if (df.at(block->GetIndex()).Get(k)) {
                out << "  " << func->GetBlock(k)->GetName();
            }
        }
        out << "\n";
    }
}
// DominatorAnalyzer end
} // namespace sc
