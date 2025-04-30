#pragma once

#include "function.hpp"
#include "index_set.hpp"
#include "operand.hpp"
#include <cassert>
#include <iostream>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sc {

class DominatorAnalyzer {
  public:
    DominatorAnalyzer(Function *f) : func(f) {}

    void ComputeDominance();
    void ComputeImmediateDominators();
    void ComputeDominanceFrontier();
    void BuildDominatorTree();

    void DumpDominators(std::ostream &out = std::cout) const;
    void DumpImmediateDominators(std::ostream &out = std::cout) const;
    void DumpDominanceFrontier(std::ostream &out = std::cout) const;

    const IndexSet &GetDominators(size_t idx) const {
        assert(idx < func->GetBlockSize());
        return dom[idx];
    }

    Block *GetImmediateDominator(size_t idx) {
        assert(idx < func->GetBlockSize());
        return idom[idx];
    }

    const IndexSet &GetDominanceFrontier(size_t idx) const {
        assert(idx < func->GetBlockSize());
        return df[idx];
    }

    std::vector<Block *> GetDominanceFrontier(Block *block) const;

    std::vector<Block *> GetDTreeSuccessor(Block *block) const {
        return dtree[block->GetIndex()];
    }

  private:
    Function *func;
    std::vector<IndexSet> dom;
    std::vector<IndexSet> df;
    std::vector<Block *> idom;
    std::vector<std::vector<Block *>> dtree;

    void BuildIndexMap();
};

class GlobalsAnalyzer {
  public:
    GlobalsAnalyzer(Function *f) : func(f) {}

    void ComputeGlobalNames();

    void DumpGlobals(std::ostream &out = std::cout) const;
    void DumpBlocks(std::ostream &out = std::cout) const;

    auto GetGlobals() const {
        return std::ranges::subrange(globals.begin(), globals.end());
    }

    auto GetBlocks(OperandBase *op) const {
        assert(blocks.contains(op));
        return std::ranges::subrange(blocks.at(op).begin(),
                                     blocks.at(op).end());
    }

  private:
    Function *func;
    std::unordered_set<OperandBase *> globals;
    std::unordered_map<OperandBase *, std::unordered_set<Block *>> blocks;
};

} // namespace sc
