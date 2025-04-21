#pragma once

#include "function.hpp"
#include <cassert>
#include <unordered_map>
#include <unordered_set>

namespace sc {

using DomMap = std::unordered_map<Block *, std::unordered_set<Block *>>;

class DominatorAnalyzer {
  public:
    DominatorAnalyzer(Function *f) : func(f) { assert(func->GetBlockSize()); }

    void ComputeDominance();
    void ComputeIDom();
    void ComputeDF();

  private:
    Function *func;
    DomMap dom;
};
} // namespace sc
