#pragma once

#include "analyzers/dominator_analyzer.hpp"
#include "analyzers/globals_analyzer.hpp"
#include "instruction.hpp"
#include "transformer.hpp"
#include <stack>
#include <unordered_map>
#include <vector>

namespace sc {

template <typename T> using stack = std::stack<T, std::vector<T>>;

class SSATransformer final : public Transformer {
    // TODO:
    // Build def-uses links
  public:
    SSATransformer(Function *_f)
        : Transformer(_f), dom(_f), globals(_f), gets(_f->GetBlockSize()) {}

    void Transform() override { RewriteInSSAForm(); }

  private:
    DominatorAnalyzer dom;
    GlobalsAnalyzer globals;

    // key: block idex
    // This ensures a unique get per operand per block
    std::vector<std::unordered_set<OperandBase *>> gets;

    // Renaming
    std::unordered_map<OperandBase *, size_t> counter;
    std::unordered_map<OperandBase *, sc::stack<OperandBase *>> name;

    // This temporary stores ensures that the reference count for
    // the old operands don't go to zero before the SSA transformation
    // is complete.
    std::unordered_set<std::shared_ptr<OperandBase>> temp_store;

    void RewriteInSSAForm();
    void Rename(Block *block);
    std::shared_ptr<OperandBase> NewDest(OperandBase *op);
    void Process(std::unordered_map<OperandBase *, size_t> &pop_count,
                 InstructionBase *instr);
};

}; // namespace sc
