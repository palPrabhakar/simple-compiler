#pragma once

#include "function.hpp"
#include "index_set.hpp"
#include <vector>
#include <iostream>

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
};
} // namespace sc
