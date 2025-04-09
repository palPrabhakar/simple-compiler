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
    AND,
    OR,
    NOT,
    // Control
    JMP,
    BR,
    CALL,
    RET,
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
    FGE,
    // Miscellaneous
    ID,
    CONST,
    PRINT,
    NOP,
    LABEL
};
// clang-format on
}; // namespace sc
