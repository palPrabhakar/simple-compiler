#pragma once

#include <vector>

#include "disjoin_set.hpp"
#include "function.hpp"
#include "operand.hpp"

namespace sc {
class SSALiveRanges {
  public:
    SSALiveRanges(Function *f) : func(f) {}

    void ComputeLiveRanges();

    bool SameRange(OperandBase *op0, OperandBase *op1);

  private:
    void MergeRanges(OperandBase *op, std::vector<OperandBase *> oprnds);
    void CreateRange(OperandBase *oprnd);

    Function *func;
    DisjointSet<std::string> ds;
};
} // namespace sc
