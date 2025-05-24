#pragma once

#include "instruction_visitor.hpp"

namespace sc {
class Interpreter final : private InstructionVisitor {
  public:
    OperandBase *ProcessInstruction(InstructionBase *instr);

  private:
    OperandBase *result = nullptr;

    // Arithmetic
    void VisitAddInstruction(AddInstruction *instr) override;
    void VisitMulInstruction(MulInstruction *instr) override;
    void VisitSubInstruction(SubInstruction *instr) override;
    void VisitDivInstruction(DivInstruction *instr) override;

    // Comparison
    void VisitEqInstruction(EqInstruction *instr) override;
    void VisitLtInstruction(LtInstruction *instr) override;
    void VisitGtInstruction(GtInstruction *instr) override;
    void VisitLeInstruction(LeInstruction *instr) override;
    void VisitGeInstruction(GeInstruction *instr) override;

    // Logic
    void VisitAndInstruction(AndInstruction *instr) override;
    void VisitOrInstruction(OrInstruction *instr) override;
    void VisitNotInstruction(NotInstruction *instr) override;

    // Floating Arithmetic
    void VisitFAddInstruction(FAddInstruction *instr) override;
    void VisitFMulInstruction(FMulInstruction *instr) override;
    void VisitFSubInstruction(FSubInstruction *instr) override;
    void VisitFDivInstruction(FDivInstruction *instr) override;

    // Floating Comparison
    void VisitFEqInstruction(FEqInstruction *instr) override;
    void VisitFLtInstruction(FLtInstruction *instr) override;
    void VisitFGtInstruction(FGtInstruction *instr) override;
    void VisitFLeInstruction(FLeInstruction *instr) override;
    void VisitFGeInstruction(FGeInstruction *instr) override;

    template <template <typename> typename Op, typename T,
              typename U = T::val_type>
    OperandBase *Interpret(auto *instr, Op<U> op = Op<U>()) const {
        auto *lop =
            static_cast<T *>(instr->GetOperand(0)->GetDef()->GetOperand(0));
        auto *rop =
            static_cast<T *>(instr->GetOperand(1)->GetDef()->GetOperand(0));

        auto value = op(lop->GetValue(), rop->GetValue());

        if constexpr (std::is_same_v<typename Op<U>::result_type,
                                     typename T::val_type>) {
            return T::GetOperand(value);
        } else {
            static_assert(std::is_same_v<typename Op<U>::result_type, bool>);
            return BoolOperand::GetOperand(value);
        }
    }
};
} // namespace sc
