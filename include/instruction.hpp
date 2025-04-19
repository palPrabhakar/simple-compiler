#pragma once
#include "opcodes.hpp"
#include "operand.hpp"
#include <cassert>
#include <iostream>
#include <ostream>
#include <vector>

namespace sc {

OpCode GetOpCodeFromStr(std::string);

class InstructionBase {
  public:
    virtual ~InstructionBase() = default;
    virtual void Dump(std::ostream &out = std::cout) const = 0;
    OpCode GetOpCode() const { return opcode; }
    size_t GetOperandSize() const { return operands.size(); }

    void SetOperand(OperandBase *oprnd) { operands.push_back(oprnd); }

    void SetOperand(OperandBase *oprnd, size_t idx) {
        assert(idx < operands.size());
        operands[idx] = oprnd;
    }

    OperandBase *GetOperand(size_t idx) {
        assert(idx < operands.size());
        return operands[idx];
    }

    static constexpr size_t OP_SIZE = 0;

  protected:
    InstructionBase(OpCode _opcode) : opcode(_opcode) {}
    std::vector<OperandBase *> operands; // non-owning pointers

  private:
    OpCode opcode;
};

class UnaryInstruction : public InstructionBase {
  public:
    UnaryInstruction(OpCode opcode) : InstructionBase(opcode) {}
    virtual void Dump(std::ostream &out = std::cout) const override = 0;
    static constexpr size_t OP_SIZE = 1;
};

class BinaryInstruction : public InstructionBase {
  public:
    BinaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}
    virtual void Dump(std::ostream &out = std::cout) const override = 0;
    static constexpr size_t OP_SIZE = 2;
};

class TernaryInstruction : public InstructionBase {
  public:
    TernaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}
    virtual void Dump(std::ostream &out = std::cout) const override = 0;
    static constexpr size_t OP_SIZE = 3;
};

// Arithmetic Instructions
class AddInstruction final : public TernaryInstruction {
  public:
    AddInstruction() : TernaryInstruction(OpCode::ADD) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class MulInstruction final : public TernaryInstruction {
  public:
    MulInstruction() : TernaryInstruction(OpCode::MUL) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class SubInstruction final : public TernaryInstruction {
  public:
    SubInstruction() : TernaryInstruction(OpCode::SUB) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class DivInstruction final : public TernaryInstruction {
  public:
    DivInstruction() : TernaryInstruction(OpCode::DIV) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Comparison Instructions
class EqInstruction final : public TernaryInstruction {
  public:
    EqInstruction() : TernaryInstruction(OpCode::EQ) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class LtInstruction final : public TernaryInstruction {
  public:
    LtInstruction() : TernaryInstruction(OpCode::LT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class GtInstruction final : public TernaryInstruction {
  public:
    GtInstruction() : TernaryInstruction(OpCode::GT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class LeInstruction final : public TernaryInstruction {
  public:
    LeInstruction() : TernaryInstruction(OpCode::LE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class GeInstruction final : public TernaryInstruction {
  public:
    GeInstruction() : TernaryInstruction(OpCode::GE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Logic Instructions
class AndInstruction final : public TernaryInstruction {
  public:
    AndInstruction() : TernaryInstruction(OpCode::AND) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class OrInstruction final : public TernaryInstruction {
  public:
    OrInstruction() : TernaryInstruction(OpCode::OR) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class NotInstruction final : public BinaryInstruction {
  public:
    NotInstruction() : BinaryInstruction(OpCode::NOT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

// Control Instruction
class JmpInstruction final : public UnaryInstruction {
    // output_label = f(input_label)
    // Changes PC by some offset
  public:
    JmpInstruction() : UnaryInstruction(OpCode::JMP) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class BranchInstruction final : public TernaryInstruction {
    // One source and two destinations
    // Source - RegOperand
    // Dest - LabelOperand
    // output_label = f(cond, input_label{0, 1})
    // Changes PC by some offset
  public:
    BranchInstruction() : TernaryInstruction(OpCode::BR) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class CallInstruction final : public InstructionBase {
    // First operand is dest
    // Rest are arguments
  public:
    CallInstruction() : InstructionBase(OpCode::CALL) {}
    void Dump(std::ostream &out = std::cout) const override;
    void SetFuncName(std::string fname) { func = fname; }
    void SetRetVal(bool val) { ret_val = val; }
    std::string GetFuncName() const { return func; }
    bool GetRetVal() const { return ret_val; }

  private:
    std::string func;
    bool ret_val;
};

class RetInstruction final : public UnaryInstruction {
  public:
    RetInstruction() : UnaryInstruction(OpCode::RET) {}
    void Dump(std::ostream &out = std::cout) const override;
};

// Memory Instructions
class AllocInstruction : public BinaryInstruction {
  public:
    AllocInstruction() : BinaryInstruction(OpCode::ALLOC) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

class FreeInstruction : public UnaryInstruction {
  public:
    FreeInstruction() : UnaryInstruction(OpCode::FREE) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class LoadInstruction : public BinaryInstruction {
  public:
    LoadInstruction() : BinaryInstruction(OpCode::LOAD) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

class StoreInstruction : public BinaryInstruction {
  public:
    StoreInstruction() : BinaryInstruction(OpCode::STORE) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class PtraddInstruction : public TernaryInstruction {
  public:
    PtraddInstruction() : TernaryInstruction(OpCode::PTRADD) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Floating-Point Arithmetic Instructions
class FAddInstruction final : public TernaryInstruction {
  public:
    FAddInstruction() : TernaryInstruction(OpCode::FADD) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FMulInstruction final : public TernaryInstruction {
  public:
    FMulInstruction() : TernaryInstruction(OpCode::FMUL) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FSubInstruction final : public TernaryInstruction {
  public:
    FSubInstruction() : TernaryInstruction(OpCode::FSUB) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FDivInstruction final : public TernaryInstruction {
  public:
    FDivInstruction() : TernaryInstruction(OpCode::FDIV) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Floating-Pointi Comparison Instructions
class FEqInstruction final : public TernaryInstruction {
  public:
    FEqInstruction() : TernaryInstruction(OpCode::FEQ) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FLtInstruction final : public TernaryInstruction {
  public:
    FLtInstruction() : TernaryInstruction(OpCode::FLT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FGtInstruction final : public TernaryInstruction {
  public:
    FGtInstruction() : TernaryInstruction(OpCode::FGT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FLeInstruction final : public TernaryInstruction {
  public:
    FLeInstruction() : TernaryInstruction(OpCode::FLE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FGeInstruction final : public TernaryInstruction {
  public:
    FGeInstruction() : TernaryInstruction(OpCode::FGE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Miscellaneous Instructions
class IdInstruction final : public BinaryInstruction {
  public:
    IdInstruction() : BinaryInstruction(OpCode::ID) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

class ConstInstruction final : public BinaryInstruction {
    // Src is ImmedOperand
  public:
    ConstInstruction() : BinaryInstruction(OpCode::CONST) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class PrintInstruction final : public InstructionBase {
  public:
    PrintInstruction() : InstructionBase(OpCode::PRINT) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class NopInstruction final : public InstructionBase {
  public:
    NopInstruction() : InstructionBase(OpCode::NOP) {}
    void Dump(std::ostream &out = std::cout) const override;
};
} // namespace sc
