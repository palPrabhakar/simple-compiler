#include "transformers/expression_simplifier.hpp"
#include <cassert>

namespace sc {

InstructionBase *
ExpressionSimplifier::ProcessInstruction(InstructionBase *instr, size_t idx) {
    ret_instr = nullptr;
    cur_idx = idx;
    instr->Visit(this);
    assert(ret_instr);
    return ret_instr;
}

void ExpressionSimplifier::VisitAddInstruction(AddInstruction *instr) {
    ProcessAdd<IntOperand>(instr);
}

void ExpressionSimplifier::VisitFAddInstruction(FAddInstruction *instr) {
    ProcessAdd<FloatOperand>(instr);
}

void ExpressionSimplifier::VisitSubInstruction(SubInstruction *instr) {
    ProcessSub<IntOperand>(instr);
}

void ExpressionSimplifier::VisitFSubInstruction(FSubInstruction *instr) {
    ProcessSub<FloatOperand>(instr);
}

void ExpressionSimplifier::VisitMulInstruction(MulInstruction *instr) {
    ProcessMul<IntOperand>(instr);
}

void ExpressionSimplifier::VisitFMulInstruction(FMulInstruction *instr) {
    ProcessMul<FloatOperand>(instr);
}

void ExpressionSimplifier::VisitDivInstruction(DivInstruction *instr) {
    ProcessDiv<IntOperand>(instr);
}

void ExpressionSimplifier::VisitFDivInstruction(FDivInstruction *instr) {
    ProcessDiv<FloatOperand>(instr);
}

void ExpressionSimplifier::VisitEqInstruction(EqInstruction *instr) {
    ProcessComparison(instr, true);
}

void ExpressionSimplifier::VisitFEqInstruction(FEqInstruction *instr) {
    ret_instr = instr;
}

void ExpressionSimplifier::VisitLtInstruction(LtInstruction *instr) {
    ProcessComparison(instr, false);
}

void ExpressionSimplifier::VisitFLtInstruction(FLtInstruction *instr) {
    ProcessComparison(instr, false);
}

void ExpressionSimplifier::VisitGtInstruction(GtInstruction *instr) {
    ProcessComparison(instr, false);
}

void ExpressionSimplifier::VisitFGtInstruction(FGtInstruction *instr) {
    ProcessComparison(instr, false);
}

void ExpressionSimplifier::VisitLeInstruction(LeInstruction *instr) {
    ProcessComparison(instr, true);
}

void ExpressionSimplifier::VisitFLeInstruction(FLeInstruction *instr) {
    ProcessComparison(instr, true);
}

void ExpressionSimplifier::VisitGeInstruction(GeInstruction *instr) {
    ProcessComparison(instr, true);
}

void ExpressionSimplifier::VisitFGeInstruction(FGeInstruction *instr) {
    ProcessComparison(instr, true);
}

void ExpressionSimplifier::VisitAndInstruction(AndInstruction *instr) {
    if (CheckValue<BoolOperand, true>(instr, 0)) {
        ReplaceWithIdInstruction(instr, 1);
    } else if (CheckValue<BoolOperand, true>(instr, 1)) {
        ReplaceWithIdInstruction(instr, 0);
    } else if (CheckValue<BoolOperand, false>(instr, 0)) {
        ReplaceWithConstInstruction<BoolOperand>(instr, false);
    } else if (CheckValue<BoolOperand, false>(instr, 1)) {
        ReplaceWithConstInstruction<BoolOperand>(instr, false);
    } else {
        ret_instr = instr;
    }
}

void ExpressionSimplifier::VisitOrInstruction(OrInstruction *instr) {
    if (CheckValue<BoolOperand, true>(instr, 0)) {
        ReplaceWithConstInstruction<BoolOperand>(instr, true);
    } else if (CheckValue<BoolOperand, true>(instr, 1)) {
        ReplaceWithConstInstruction<BoolOperand>(instr, true);
    } else if (CheckValue<BoolOperand, false>(instr, 0)) {
        ReplaceWithIdInstruction(instr, 1);
    } else if (CheckValue<BoolOperand, false>(instr, 1)) {
        ReplaceWithIdInstruction(instr, 0);
    } else {
        ret_instr = instr;
    }
}

} // namespace sc
