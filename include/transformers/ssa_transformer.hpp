#pragma once

#include "analyzers/dominator_analyzer.hpp"
#include "analyzers/globals_analyzer.hpp"
#include "transformer.hpp"
#include <unordered_map>
#include <stack>
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
    std::vector<std::unordered_set<OperandBase *>> gets;
    std::unordered_map<OperandBase *, size_t> counter;
    std::unordered_map<OperandBase *, sc::stack<OperandBase *>> name;

    void RewriteInSSAForm();
    void Rename(Block *block);

    OperandBase *NewDest(OperandBase *op);
};

}; // namespace sc
