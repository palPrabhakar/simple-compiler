#pragma once

#include "analyzers/dominator_analyzer.hpp"
#include "transformer.hpp"

namespace sc {
class DCETransformer final : public Transformer {
  public:
    DCETransformer(Function *f)
        : Transformer(f), imarks(f->GetBlockSize()),
          bmarks(f->GetBlockSize(), false), dom(f, true) {}
    void Transform() override;

  private:
    std::vector<std::vector<bool>> imarks;
    std::vector<bool> bmarks;
    DominatorAnalyzer dom;

    void Mark();
    void Sweep();
    bool Critical(InstructionBase *instr);
    bool CheckAndMark(InstructionBase *instr);

    bool Check(InstructionBase *instr) {
        return imarks[instr->GetBlock()->GetIndex()][instr->GetIndex()];
    }

    void Mark(InstructionBase *instr) {
        imarks[instr->GetBlock()->GetIndex()][instr->GetIndex()] = true;
        bmarks[instr->GetBlock()->GetIndex()] = true;
    }
};
} // namespace sc
