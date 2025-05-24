#include "transformers/interpreter.hpp"
#include "operand.hpp"
#include <memory>

namespace sc {
std::unique_ptr<OperandBase>
Interpreter::ProcessInstruction(InstructionBase *instr) {
    // instr->Visit(this);
    return std::move(result);
}

// void Interpreter::VisitAddInstruction(AddInstruction *instr) {
//     auto *lop = static_cast<IntOperand *>(
//         instr->GetOperand(0)->GetDef()->GetOperand(0));
//     auto *rop = static_cast<IntOperand *>(
//         instr->GetOperand(1)->GetDef()->GetOperand(0));

//     [[maybe_unused]] auto value = lop->GetValue() + rop->GetValue();

// }

} // namespace sc
