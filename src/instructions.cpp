#include "instructions.hpp"
#include "opcodes.hpp"
#include <cassert>
#include <iostream>
#include <unordered_map>

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
    // Miscellaneous
    {"id", OpCode::ID},
    {"print", OpCode::PRINT},
    {"const", OpCode::CONST},
    {"nop", OpCode::NOP},
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
    {"fge", OpCode::FGE}
};
// clang-format on

OpCode GetOpCodeFromStr(std::string op_code) {
    assert(str_to_opcodes.contains(op_code) &&
           "GetOpCodeFromStr: Invalid opcode str\n");
    return str_to_opcodes.at(op_code);
}

} // namespace sc
