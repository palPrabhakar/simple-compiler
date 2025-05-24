#include "instruction.hpp"
#include "instruction_visitor.hpp"
#include "operand.hpp"
#include <cassert>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <unordered_map>

#define PRINT_HELPER3(name)                                                    \
    out << dest->GetName() << ":"                                              \
        << " " << dest->GetStrType() << " = " #name " "                        \
        << operands[0]->GetName() << " " << operands[1]->GetName() << ";\n";

#define PRINT_HELPER2(name)                                                    \
    out << dest->GetName() << ":"                                              \
        << " " << dest->GetStrType() << " = " #name " "                        \
        << operands[0]->GetName() << ";\n";

#define ADD_VISITOR(name)                                                      \
    void name##Instruction::Visit(InstructionVisitor *visitor) {               \
        visitor->Visit##name##Instruction(this);                               \
    }

namespace sc {
// clang-format off
static const std::unordered_map<std::string, Opcode> str_to_Opcodes = {
    // Arithmetic
    {"add", Opcode::ADD},
    {"mul", Opcode::MUL},
    {"sub", Opcode::SUB},
    {"div", Opcode::DIV},
    // Comparison
    {"eq", Opcode::EQ},
    {"lt", Opcode::LT},
    {"gt", Opcode::GT},
    {"le", Opcode::LE},
    {"ge", Opcode::GE},
    // Logic
    {"not", Opcode::NOT},
    {"and", Opcode::AND},
    {"or", Opcode::OR},
    // Control
    {"jmp", Opcode::JMP},
    {"br", Opcode::BR},
    {"call", Opcode::CALL},
    {"ret", Opcode::RET},
    // SSA
    {"get", Opcode::GET},
    {"set", Opcode::SET},
    {"undef", Opcode::UNDEF},
    // Memory
    {"alloc", Opcode::ALLOC},
    {"free", Opcode::FREE},
    {"load", Opcode::LOAD},
    {"store", Opcode::STORE},
    {"ptradd", Opcode::PTRADD},
    // Floating
    {"fadd", Opcode::FADD},
    {"fmul", Opcode::FMUL},
    {"fsub", Opcode::FSUB},
    {"fdiv", Opcode::FDIV},
    // FComparisons
    {"feq", Opcode::FEQ},
    {"flt", Opcode::FLT},
    {"fle", Opcode::FLE},
    {"fgt", Opcode::FGT},
    {"fge", Opcode::FGE},
    // Miscellaneous
    {"id", Opcode::ID},
    {"const", Opcode::CONST},
    {"print", Opcode::PRINT},
    {"nop", Opcode::NOP},
};
// clang-format on

Opcode GetOpcodeFromStr(std::string op_code) {
    assert(str_to_Opcodes.contains(op_code) &&
           "GetOpcodeFromStr: Invalid Opcode str\n");
    return str_to_Opcodes.at(op_code);
}

// Arithmetic Instructions
// clang-format off
void AddInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(add) }
void MulInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(mul) }
void SubInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(sub) }
void DivInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(div) }

ADD_VISITOR(Add)
ADD_VISITOR(Mul)
ADD_VISITOR(Sub)
ADD_VISITOR(Div)

// clang-format on

// Comparsion Instructions
// clang-format off
void EqInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(eq) }
void LtInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(lt) }
void GtInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(gt) }
void LeInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(le) }
void GeInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(ge) }

ADD_VISITOR(Eq)
ADD_VISITOR(Lt)
ADD_VISITOR(Gt)
ADD_VISITOR(Le)
ADD_VISITOR(Ge)
// clang-format on

// Logic Instructions
// clang-format off
void AndInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(and) }
void OrInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(or) }
void NotInstruction::Dump(std::ostream &out) const { PRINT_HELPER2(not) }

ADD_VISITOR(And)
ADD_VISITOR(Or)
ADD_VISITOR(Not)
// clang-format on

// Control Instructions
void JmpInstruction::Dump(std::ostream &out) const {
    out << "jmp ." << jmp_lbl->GetName() << ";\n";
}

void BranchInstruction::Dump(std::ostream &out) const {
    out << "br " << operands[0]->GetName() << " ." << true_lbl->GetName()
        << " ." << false_lbl->GetName() << ";\n";
}

void CallInstruction::Dump(std::ostream &out) const {
    if (ret_val) {
        out << dest->GetName() << ": " << dest->GetStrType() << " = ";
    }
    out << "call @" << func;
    for (size_t i : std::views::iota(0ul, operands.size())) {
        out << " " << operands[i]->GetName();
    }
    out << ";\n";
}

void RetInstruction::Dump(std::ostream &out) const {
    out << "ret";
    if (operands.size() && operands[0] != VoidOperand::GetVoidOperand()) {
        out << " " << operands[0]->GetName();
    }
    out << ";\n";
}

// clang-format off
ADD_VISITOR(Jmp)
ADD_VISITOR(Branch)
ADD_VISITOR(Call)
ADD_VISITOR(Ret)
// clang-format on

// Control Instructions
void SetInstruction::Dump(std::ostream &out) const {
    out << "set " << shadow->GetName() << " " << operands[0]->GetName()
        << ";\n";
}

void GetInstruction::Dump(std::ostream &out) const {
    out << dest->GetName() << ":"
        << " " << dest->GetStrType() << " = get;\n";
}

void UndefInstruction::Dump(std::ostream &out) const {
    out << dest->GetName() << ":"
        << " " << dest->GetStrType() << " = undef;\n";
}

// clang-format off
ADD_VISITOR(Set)
ADD_VISITOR(Get)
ADD_VISITOR(Undef)
// clang-format on

// Memory Instructions
void AllocInstruction::Dump(std::ostream &out) const { PRINT_HELPER2(alloc); }

void FreeInstruction::Dump(std::ostream &out) const {
    out << "free " << operands[0]->GetName() << ";\n";
}

void LoadInstruction::Dump(std::ostream &out) const { PRINT_HELPER2(load) }

void StoreInstruction::Dump(std::ostream &out) const {
    out << "store";
    for (size_t i : std::views::iota(0ul, operands.size())) {
        out << " " << operands[i]->GetName();
    }
    out << ";\n";
}

void PtraddInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(ptradd) }

// clang-format off
ADD_VISITOR(Alloc)
ADD_VISITOR(Free)
ADD_VISITOR(Load)
ADD_VISITOR(Store)
ADD_VISITOR(Ptradd)
// clang-format on

// Floating-Point Arithmetic Instructions
void FAddInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fadd) }
void FMulInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fmul) }
void FSubInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fsub) }
void FDivInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fdiv) }

// clang-format off
ADD_VISITOR(FAdd)
ADD_VISITOR(FMul)
ADD_VISITOR(FSub)
ADD_VISITOR(FDiv)
// clang-format of

// Floating-Point Comparsion Instructions
void FEqInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(feq) }
void FLtInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(flt) }
void FGtInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fgt) }
void FLeInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fle) }
void FGeInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fge) }

//clang-format off
ADD_VISITOR(FEq)
ADD_VISITOR(FLt)
ADD_VISITOR(FGt)
ADD_VISITOR(FLe)
ADD_VISITOR(FGe)
//clang-format on

// Miscellaneous Instructions
void IdInstruction::Dump(std::ostream &out) const { PRINT_HELPER2(id) }

void ConstInstruction::Dump(std::ostream &out) const {
    auto *src = operands[0];
    auto type = dest->GetStrType();
    out << dest->GetName() << ":"
        << " " << type << " = const ";
    switch (src->GetType()) {
    case DataType::INT: {
        auto int_src = static_cast<IntOperand *>(src);
        out << int_src->GetValue();
    } break;
    case DataType::FLOAT: {
        auto float_src = static_cast<FloatOperand *>(src);
        out << std::fixed << std::setprecision(15) << float_src->GetValue();
        out.unsetf(std::ios::fixed);
    } break;
    case DataType::BOOL: {
        auto bool_src = static_cast<BoolOperand *>(src);
        out << (bool_src->GetValue() ? "true" : "false");
    } break;
    default:
        assert(false);
    }
    out << ";\n";
}

void PrintInstruction::Dump(std::ostream &out) const {
    out << "print";
    for (auto op : operands) {
        out << " " << op->GetName();
    }
    out << ";\n";
}

void NopInstruction::Dump(std::ostream &out) const { out << "nop;\n"; }

//clang-format off
ADD_VISITOR(Id)
ADD_VISITOR(Const)
ADD_VISITOR(Print)
ADD_VISITOR(Nop)
//clang-format on

} // namespace sc
