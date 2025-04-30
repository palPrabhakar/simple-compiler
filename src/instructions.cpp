#include "instruction.hpp"
#include "opcodes.hpp"
#include "operand.hpp"
#include <cassert>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <unordered_map>

#define PRINT_HELPER3(name)                                                    \
    out << operands[0]->GetName() << ":"                                       \
        << " " << operands[0]->GetStrType() << " = " #name " "                 \
        << operands[1]->GetName() << " " << operands[2]->GetName() << ";\n";

#define PRINT_HELPER2(name)                                                    \
    out << operands[0]->GetName() << ":"                                       \
        << " " << operands[0]->GetStrType() << " = " #name " "                 \
        << operands[1]->GetName() << ";\n";

namespace sc {
// clang-format off
static const std::unordered_map<std::string, OpCode> str_to_opcodes = {
    // Arithmetic
    {"add", OpCode::ADD},
    {"mul", OpCode::MUL},
    {"sub", OpCode::SUB},
    {"div", OpCode::DIV},
    // Comparison
    {"eq", OpCode::EQ},
    {"lt", OpCode::LT},
    {"gt", OpCode::GT},
    {"le", OpCode::LE},
    {"ge", OpCode::GE},
    // Logic
    {"not", OpCode::NOT},
    {"and", OpCode::AND},
    {"or", OpCode::OR},
    // Control
    {"jmp", OpCode::JMP},
    {"br", OpCode::BR},
    {"call", OpCode::CALL},
    {"ret", OpCode::RET},
    // SSA
    {"get", OpCode::GET},
    {"set", OpCode::SET},
    // Memory
    {"alloc", OpCode::ALLOC},
    {"free", OpCode::FREE},
    {"load", OpCode::LOAD},
    {"store", OpCode::STORE},
    {"ptradd", OpCode::PTRADD},
    // Floating
    {"fadd", OpCode::FADD},
    {"fmul", OpCode::FMUL},
    {"fsub", OpCode::FSUB},
    {"fdiv", OpCode::FDIV},
    // FComparisons
    {"feq", OpCode::FEQ},
    {"flt", OpCode::FLT},
    {"fle", OpCode::FLE},
    {"fgt", OpCode::FGT},
    {"fge", OpCode::FGE},
    // Miscellaneous
    {"id", OpCode::ID},
    {"const", OpCode::CONST},
    {"print", OpCode::PRINT},
    {"nop", OpCode::NOP},
};
// clang-format on

OpCode GetOpCodeFromStr(std::string op_code) {
    assert(str_to_opcodes.contains(op_code) &&
           "GetOpCodeFromStr: Invalid opcode str\n");
    return str_to_opcodes.at(op_code);
}

// Arithmetic Instructions
void AddInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(add) }
void MulInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(mul) }
void SubInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(sub) }
void DivInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(div) }

// Comparsion Instructions
void EqInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(eq) }
void LtInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(lt) }
void GtInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(gt) }
void LeInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(le) }
void GeInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(ge) }

// Logic Instructions
// clang-format off
void AndInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(and) }
void OrInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(or) }
void NotInstruction::Dump(std::ostream &out) const { PRINT_HELPER2(not) }
// clang-format on

// Control Instructions
void JmpInstruction::Dump(std::ostream &out) const {
    out << "jmp ." << operands[0]->GetName() << ";\n";
}

void BranchInstruction::Dump(std::ostream &out) const {
    out << "br " << operands[2]->GetName() << " ." << operands[0]->GetName()
        << " ." << operands[1]->GetName() << ";\n";
}

void CallInstruction::Dump(std::ostream &out) const {
    if (ret_val) {
        out << operands[0]->GetName() << ": " << operands[0]->GetStrType()
            << " = ";
    }
    out << "call @" << func;
    for (size_t i :
         std::views::iota(static_cast<size_t>(ret_val), operands.size())) {
        out << " " << operands[i]->GetName();
    }
    out << ";\n";
}

void RetInstruction::Dump(std::ostream &out) const {
    out << "ret";
    if (operands.size()) {
        out << " " << operands[0]->GetName();
    }
    out << ";\n";
}

// Control Instructions
void SetInstruction::Dump(std::ostream &out) const {
    out << "set";
    for (auto i : std::views::iota(0ul, operands.size())) {
        out << " " << operands[i]->GetName();
    }
    out << ";\n";
}

void GetInstruction::Dump(std::ostream &out) const {
    out << operands[0]->GetName() << ":"
        << " " << operands[0]->GetStrType() << " = get;\n";
}

// Memory Instructions
void AllocInstruction::Dump(std::ostream &out) const {
    out << operands[0]->GetName() << ": " << operands[0]->GetStrType()
        << " = alloc " << operands[1]->GetName() << ";\n";
}

void FreeInstruction::Dump(std::ostream &out) const {
    out << "free " << operands[0]->GetName() << ";\n";
}

void LoadInstruction::Dump(std::ostream &out) const {
    out << operands[0]->GetName() << ": " << operands[0]->GetStrType()
        << " = load " << operands[1]->GetName() << ";\n";
}

void StoreInstruction::Dump(std::ostream &out) const {
    out << "store";
    for (size_t i : std::views::iota(0ul, operands.size())) {
        out << " " << operands[i]->GetName();
    }
    out << ";\n";
}

void PtraddInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(ptradd) }

// Floating-Point Arithmetic Instructions
void FAddInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fadd) }
void FMulInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fmul) }
void FSubInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fsub) }
void FDivInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fdiv) }

// Floating-Point Comparsion Instructions
void FEqInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(feq) }
void FLtInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(flt) }
void FGtInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fgt) }
void FLeInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fle) }
void FGeInstruction::Dump(std::ostream &out) const { PRINT_HELPER3(fge) }

// Miscellaneous Instructions
void IdInstruction::Dump(std::ostream &out) const { PRINT_HELPER2(id) }

void ConstInstruction::Dump(std::ostream &out) const {
    auto dest = operands[0];
    auto src = operands[1];
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
} // namespace sc
