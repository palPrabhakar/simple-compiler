#pragma once

#include "function.hpp"
#include "index_set.hpp"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace sc {

// using DomMap = std::unordered_map<Block *, std::unordered_set<Block *>>;
using DomMap = std::vector<IndexSet>;
using IdomMap = std::vector<Block *>;
using IndexMap = std::unordered_map<Block *, size_t>;

class DominatorAnalyzer {
  public:
    DominatorAnalyzer(Function *f) : func(f) {
        assert(func->GetBlockSize());
        BuildIndexMap();
    }

    void ComputeDominance();
    void ComputeImmediateDominators();
    void ComputeDominanceFrontier();

    void DumpDominators(std::ostream &out = std::cout);
    void DumpImmediateDominators(std::ostream &out = std::cout);

    const IndexSet &GetDominators(size_t idx) const {
        assert(idx < func->GetBlockSize());
        return dom[idx];
    }

    Block *&GetImmediateDominator(size_t idx) {
        assert(idx < func->GetBlockSize());
        return idom[idx];
    }

  private:
    Function *func;
    DomMap dom;
    IdomMap idom;
    IndexMap imap;

    void BuildIndexMap();
};
} // namespace sc
