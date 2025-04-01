#pragma once

namespace sc {
// clang-format off
enum class OpCode {
    // Arithmetic
    ADD,
    MUL,
    SUB,
    DIV,
    // Comparison
    EQ,
    LT,
    GT,
    LE,
    GE,
    // Logic
    NOT,
    AND,
    OR,
    // Control
    JMP,
    BR,
    CALL,
    RET,
    // Miscellaneous
    ID,
    PRINT,
    CONST,
    NOP,
    // SSA
    SET,
    GET,
    // Memory
    ALLOC,
    FREE,
    LOAD,
    STORE,
    PTRADD,
    // Floating
    FADD,
    FMUL,
    FSUB,
    FDIV,
    // FComparisons
    FEQ,
    FLT,
    FLE,
    FGT,
    FGE
};
// clang-format on
}; // namespace sc
