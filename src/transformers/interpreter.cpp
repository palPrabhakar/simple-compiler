#include "transformers/interpreter.hpp"
#include "operand.hpp"
#include <functional>

namespace sc {
OperandBase *Interpreter::ProcessInstruction(InstructionBase *instr) {
    instr->Visit(this);
    return result;
}

void Interpreter::VisitAddInstruction(AddInstruction *instr) {
    result = Interpret<std::plus, IntOperand>(instr);
}

void Interpreter::VisitMulInstruction(MulInstruction *instr) {
    result = Interpret<std::multiplies, IntOperand>(instr);
}

void Interpreter::VisitSubInstruction(SubInstruction *instr) {
    result = Interpret<std::minus, IntOperand>(instr);
}

void Interpreter::VisitDivInstruction(DivInstruction *instr) {
    result = Interpret<std::divides, IntOperand>(instr);
}

void Interpreter::VisitEqInstruction(EqInstruction *instr) {
    result = Interpret<std::equal_to, IntOperand>(instr);
}

void Interpreter::VisitLtInstruction(LtInstruction *instr) {
    result = Interpret<std::less, IntOperand>(instr);
}

void Interpreter::VisitGtInstruction(GtInstruction *instr) {
    result = Interpret<std::greater, IntOperand>(instr);
}

void Interpreter::VisitLeInstruction(LeInstruction *instr) {
    result = Interpret<std::less_equal, IntOperand>(instr);
}

void Interpreter::VisitGeInstruction(GeInstruction *instr) {
    result = Interpret<std::greater_equal, IntOperand>(instr);
}

void Interpreter::VisitAndInstruction(AndInstruction *instr) {
    result = Interpret<std::logical_and, BoolOperand>(instr);
};

void Interpreter::VisitOrInstruction(OrInstruction *instr) {
    result = Interpret<std::logical_or, BoolOperand>(instr);
}

void Interpreter::VisitNotInstruction(NotInstruction *instr) {
    auto *op = static_cast<BoolOperand *>(
        instr->GetOperand(0)->GetDef()->GetOperand(0));
    result = BoolOperand::GetOperand(!op->GetValue());
}

void Interpreter::VisitFAddInstruction(FAddInstruction *instr) {
    result = Interpret<std::plus, FloatOperand>(instr);
}

void Interpreter::VisitFMulInstruction(FMulInstruction *instr) {
    result = Interpret<std::multiplies, FloatOperand>(instr);
}

void Interpreter::VisitFSubInstruction(FSubInstruction *instr) {
    result = Interpret<std::minus, FloatOperand>(instr);
}

void Interpreter::VisitFDivInstruction(FDivInstruction *instr) {
    result = Interpret<std::divides, FloatOperand>(instr);
}

void Interpreter::VisitFEqInstruction(FEqInstruction *instr) {
    result = Interpret<std::equal_to, FloatOperand>(instr);
}

void Interpreter::VisitFLtInstruction(FLtInstruction *instr) {
    result = Interpret<std::less, FloatOperand>(instr);
}

void Interpreter::VisitFGtInstruction(FGtInstruction *instr) {
    result = Interpret<std::greater, FloatOperand>(instr);
}

void Interpreter::VisitFLeInstruction(FLeInstruction *instr) {
    result = Interpret<std::less_equal, FloatOperand>(instr);
}

void Interpreter::VisitFGeInstruction(FGeInstruction *instr) {
    result = Interpret<std::greater_equal, FloatOperand>(instr);
}

} // namespace sc
