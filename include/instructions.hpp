#pragma once

// clang-format off
enum class OpCodes {
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

class InstructionBase {
  public:
    virtual void emit_code_str();
    virtual ~InstructionBase();

  protected:
    InstructionBase() = delete;
};
