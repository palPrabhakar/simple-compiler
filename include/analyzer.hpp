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
using IndexMap = std::unordered_map<Block *, size_t>;

class DominatorAnalyzer {
  public:
    DominatorAnalyzer(Function *f) : func(f) {
        assert(func->GetBlockSize());
        BuildIndexMap();
    }

    void ComputeDominance();
    void ComputeIDom();
    void ComputeDF();
    void DumpDominators(std::ostream &out = std::cout);

    const IndexSet &GetDominators(size_t idx) const {
        assert(idx < func->GetBlockSize());
        return dom[idx];
    }

  private:
    Function *func;
    DomMap dom;
    IndexMap imap;

    void BuildIndexMap();
};
} // namespace sc
