#pragma once

#include "block.hpp"
#include "instruction.hpp"
#include "instruction_visitor.hpp"
#include <memory>

namespace sc {
class Function;

/*
 * NOTE:
 * Not taking into account floating point rounding errors
 */
class ExpressionSimplifier final : private InstructionVisitor {
  public:
    InstructionBase *ProcessInstruction(InstructionBase *instr, size_t idx);

  private:
    InstructionBase *ret_instr;
    size_t cur_idx;

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

    template <typename T, typename T::val_type value>
    bool CheckValue(InstructionBase *instr, size_t i) {
        auto *def = instr->GetOperand(i)->GetDef();
        // TODO:
        // This check is a workaround since the function
        // arguments have no defining instructions
        if (def && def->GetOpcode() == Opcode::CONST &&
            static_cast<T *>(def->GetOperand(0))->GetValue() == value) {
            return true;
        }
        return false;
    }

    bool CheckEqual(InstructionBase *instr) {
        return instr->GetOperand(0) == instr->GetOperand(1);
    }

    void ReplaceWithIdInstruction(InstructionBase *instr, size_t i) {
        auto n_inst = std::make_unique<IdInstruction>();
        SetDestAndDef(n_inst.get(), instr->GetDest());
        SetOperandAndUse(n_inst.get(), instr->GetOperand(i));
        ret_instr = n_inst.get();
        instr->GetBlock()->AddInstruction(std::move(n_inst), cur_idx);
    }

    template <typename T>
    void ReplaceWithConstInstruction(InstructionBase *instr,
                                     T::val_type value) {
        auto n_inst = std::make_unique<ConstInstruction>();
        SetDestAndDef(n_inst.get(), instr->GetDest());
        SetOperandAndUse(n_inst.get(), T::GetOperand(value));
        ret_instr = n_inst.get();
        instr->GetBlock()->AddInstruction(std::move(n_inst), cur_idx);
    }

    template <typename T> void ProcessAdd(auto *instr) {
        // change with id
        if (CheckValue<T, static_cast<T::val_type>(0)>(instr, 0)) {
            ReplaceWithIdInstruction(instr, 1);
        } else if (CheckValue<T, static_cast<T::val_type>(0)>(instr, 1)) {
            ReplaceWithIdInstruction(instr, 0);
        } else {
            ret_instr = instr;
        }
    }

    template <typename T> void ProcessSub(auto *instr) {
        if (CheckEqual(instr)) {
            ReplaceWithConstInstruction<T>(instr, 0);
        } else if (CheckValue<T, static_cast<T::val_type>(0)>(instr, 1)) {
            ReplaceWithIdInstruction(instr, 0);
        } else {
            ret_instr = instr;
        }
    }

    template <typename T> void ProcessMul(auto *instr) {
        if (CheckValue<T, static_cast<T::val_type>(0)>(instr, 0)) {
            ReplaceWithConstInstruction<T>(instr, 0);
        } else if (CheckValue<T, static_cast<T::val_type>(0)>(instr, 1)) {
            ReplaceWithConstInstruction<T>(instr, 0);
        } else if (CheckValue<T, static_cast<T::val_type>(1)>(instr, 0)) {
            ReplaceWithIdInstruction(instr, 1);
        } else if (CheckValue<T, static_cast<T::val_type>(1)>(instr, 1)) {
            ReplaceWithIdInstruction(instr, 0);
        } else {
            ret_instr = instr;
        }
    }

    template <typename T> void ProcessDiv(auto *instr) {
        if (CheckValue<T, static_cast<T::val_type>(1)>(instr, 1)) {
            ReplaceWithIdInstruction(instr, 0);
        } else {
            ret_instr = instr;
        }
    }

    void ProcessComparison(auto *instr, bool result) {
        if (CheckEqual(instr)) {
            ReplaceWithConstInstruction<BoolOperand>(instr, result);
        } else {
            ret_instr = instr;
        }
    }
};
} // namespace sc
