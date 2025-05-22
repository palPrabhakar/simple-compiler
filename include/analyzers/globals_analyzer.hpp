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

    auto GetGlobals() {
        return std::ranges::subrange(globals.begin(), globals.end());
    }

    auto GetBlocks(OperandBase *op) {
        static std::unordered_set<Block *> empty;
        if (blocks.contains(op)) {
            return std::ranges::subrange(blocks[op].begin(),
                                         blocks[op].end());
        } else {
            return std::ranges::subrange(empty.end(), empty.end());
        }
    }

  private:
    Function *func;
    std::unordered_set<OperandBase *> globals;
    std::unordered_map<OperandBase *, std::unordered_set<Block *>> blocks;

    void Process(std::unordered_set<OperandBase *> &var_kill,
                 InstructionBase *instr, Block *block);
};
} // namespace sc
