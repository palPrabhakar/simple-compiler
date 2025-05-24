#pragma once

#include "instruction_visitor.hpp"
#include <memory>

namespace sc {

class Interpreter : public InstructionVisitor {
  public:
    std::unique_ptr<OperandBase> ProcessInstruction(InstructionBase *instr);

    // void VisitAddInstruction(AddInstruction *instr) override;
    // void VisitMulInstruction(MulInstruction *instr) override;
    // void VisitSubInstruction(SubInstruction *instr) override;
    // void VisitDivInstruction(DivInstruction *instr) override;

    // // Comparison
    // void VisitEqInstruction(EqInstruction *instr) override;
    // void VisitLtInstruction(LtInstruction *instr) override;
    // void VisitGtInstruction(GtInstruction *instr) override;
    // void VisitLeInstruction(LeInstruction *instr) override;
    // void VisitGeInstruction(GeInstruction *instr) override;

    // // Logic
    // void VisitAndInstruction(AndInstruction *instr) override;
    // void VisitOrInstruction(OrInstruction *instr) override;
    // void VisitNotInstruction(NotInstruction *instr) override;

    // // Floating Arithmetic
    // void VisitFAddInstruction(FAddInstruction *instr) override;
    // void VisitFMulInstruction(FMulInstruction *instr) override;
    // void VisitFSubInstruction(FSubInstruction *instr) override;
    // void VisitFDivInstruction(FDivInstruction *instr) override;

    // // Floating Comparison
    // void VisitFEqInstruction(FAddInstruction *instr) override;
    // void VisitFLtInstruction(FMulInstruction *instr) override;
    // void VisitFGtInstruction(FSubInstruction *instr) override;
    // void VisitFLeInstruction(FDivInstruction *instr) override;
    // void VisitFGeInstruction(FDivInstruction *instr) override;

  private:
    std::unique_ptr<OperandBase> result = nullptr;
};
} // namespace sc
