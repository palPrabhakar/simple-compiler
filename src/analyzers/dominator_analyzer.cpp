#include "analyzers/dominator_analyzer.hpp"
#include <ranges>

namespace sc {
// DominatorAnalyzer begin
void DominatorAnalyzer::ComputeDominance() {
    for (auto _ : std::views::iota(0ul, func->GetBlockSize())) {
        dom.emplace_back(func->GetBlockSize(), true);
    }

    auto ridx = cfg->GetRoot()->GetIndex();
    dom[ridx] = IndexSet(func->GetBlockSize(), false);
    dom[ridx].Set(ridx);

    auto rpo = GetReversePostOrder(cfg.get());

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto *block : rpo) {
            // Only entry block has no predecessor. Entry block's
            // dom set only contains itself
            if (cfg->HasPredecessors(block)) {
                auto predecessors = cfg->GetPredecessors(block);

                IndexSet ns(dom[predecessors[0]->GetIndex()]);

                for (auto k : std::views::iota(1ul, predecessors.size())) {
                    ns = ns & dom[predecessors[k]->GetIndex()];
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

    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
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

    // The algorithm doesn't specify any ordering on
    // the blocks during processing.
    // Start with 0-index since in reverse cfg block0 will
    // have predecessors.
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        auto predecessors = cfg->GetPredecessors(block);
        if (predecessors.size() > 1) {
            for (auto *runner : predecessors) {
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
    for (auto i : std::views::iota(0ul, idom.size())) {
        if (idom[i]) {
            dtree[idom[i]->GetIndex()].push_back(func->GetBlock(i));
        }
    }
}

void DominatorAnalyzer::BuildRPODominatorTree() {
    // successor's are in reverse postorder
    if (idom.size() != func->GetBlockSize()) {
        ComputeImmediateDominators();
    }

    dtree = std::vector<std::vector<Block *>>(func->GetBlockSize());
    auto rpo = GetReversePostOrder(cfg.get());
    for (auto *blk : rpo) {
        if (idom[blk->GetIndex()]) {
            dtree[idom[blk->GetIndex()]->GetIndex()].push_back(blk);
        }
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
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        block = func->GetBlock(i);
        out << block->GetName() << ":";
        if (idom[i]) {
            out << "  " << idom[i]->GetName() << "\n";
        }
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
