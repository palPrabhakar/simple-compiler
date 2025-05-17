#pragma once

#include "function.hpp"
#include "operand.hpp"
#include <cassert>
#include <iostream>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

namespace sc {

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

    void Process(std::unordered_set<OperandBase *> &var_kill,
                 InstructionBase *instr, Block *block, size_t start,
                 bool dest = true);
};

} // namespace sc
