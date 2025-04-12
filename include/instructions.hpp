#pragma once
#include "opcodes.hpp"
#include "operands.hpp"
#include <cassert>
#include <iostream>
#include <ostream>
#include <vector>

namespace sc {

OpCode GetOpCodeFromStr(std::string);

class InstructionBase {
  public:
    virtual ~InstructionBase() = default;
    virtual void Dump(std::ostream &out = std::cout) = 0;
    OpCode GetOpCode() const { return opcode; }
    size_t GetOperandSize() const { return operands.size(); }

    void SetOperand(OperandBase *oprnd) { operands.push_back(oprnd); }

    void SetOperand(OperandBase *oprnd, size_t idx) {
        assert(idx < operands.size() &&
               "idx out of bounds InstructionBase::SetOperands\n");
        operands[idx] = oprnd;
    }

    OperandBase *GetOperand(size_t idx) {
        assert(idx < operands.size());
        return operands[idx];
    }

  protected:
    InstructionBase(OpCode _opcode) : opcode(_opcode) {}
    std::vector<OperandBase *> operands; // non-owning pointers

  private:
    OpCode opcode;
};

class UnaryInstruction : public InstructionBase {
  public:
    UnaryInstruction(OpCode opcode) : InstructionBase(opcode) {}
    virtual void Dump(std::ostream &out = std::cout) override = 0;
};

class BinaryInstruction : public InstructionBase {
  public:
    BinaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}
    virtual void Dump(std::ostream &out = std::cout) override = 0;
};

class TernaryInstruction : public InstructionBase {
  public:
    TernaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}
    virtual void Dump(std::ostream &out = std::cout) override = 0;
};

// Arithmetic Instructions
class AddInstruction final : public TernaryInstruction {
  public:
    AddInstruction() : TernaryInstruction(OpCode::ADD) {}
    void Dump(std::ostream &out = std::cout) override;
};

class MulInstruction final : public TernaryInstruction {
  public:
    MulInstruction() : TernaryInstruction(OpCode::MUL) {}
    void Dump(std::ostream &out = std::cout) override;
};

class SubInstruction final : public TernaryInstruction {
  public:
    SubInstruction() : TernaryInstruction(OpCode::SUB) {}
    void Dump(std::ostream &out = std::cout) override;
};

class DivInstruction final : public TernaryInstruction {
  public:
    DivInstruction() : TernaryInstruction(OpCode::DIV) {}
    void Dump(std::ostream &out = std::cout) override;
};

// Comparison Instructions
class EqInstruction final : public TernaryInstruction {
  public:
    EqInstruction() : TernaryInstruction(OpCode::EQ) {}
    void Dump(std::ostream &out = std::cout) override;
};

class LtInstruction final : public TernaryInstruction {
  public:
    LtInstruction() : TernaryInstruction(OpCode::LT) {}
    void Dump(std::ostream &out = std::cout) override;
};

class GtInstruction final : public TernaryInstruction {
  public:
    GtInstruction() : TernaryInstruction(OpCode::GT) {}
    void Dump(std::ostream &out = std::cout) override;
};

class LeInstruction final : public TernaryInstruction {
  public:
    LeInstruction() : TernaryInstruction(OpCode::LE) {}
    void Dump(std::ostream &out = std::cout) override;
};

class GeInstruction final : public TernaryInstruction {
  public:
    GeInstruction() : TernaryInstruction(OpCode::GE) {}
    void Dump(std::ostream &out = std::cout) override;
};

// Logic Instructions
class AndInstruction final : public TernaryInstruction {
  public:
    AndInstruction() : TernaryInstruction(OpCode::AND) {}
    void Dump(std::ostream &out = std::cout) override;
};

class OrInstruction final : public TernaryInstruction {
  public:
    OrInstruction() : TernaryInstruction(OpCode::OR) {}
    void Dump(std::ostream &out = std::cout) override;
};

class NotInstruction final : public BinaryInstruction {
  public:
    NotInstruction() : BinaryInstruction(OpCode::NOT) {}
    void Dump(std::ostream &out = std::cout) override;
};

// Control Instruction
class JmpInstruction final : public UnaryInstruction {
  public:
    JmpInstruction() : UnaryInstruction(OpCode::JMP) {}
    void Dump(std::ostream &out = std::cout) override;
};

class BranchInstruction final : public TernaryInstruction {
    // One source and two destinations
    // Source - RegOperand
    // Dest - LabelOperand
  public:
    BranchInstruction() : TernaryInstruction(OpCode::BR) {}
    void Dump(std::ostream &out = std::cout) override;
};

class CallInstruction final : public InstructionBase {
    // First operand is dest
    // Rest are arguments
  public:
    CallInstruction() : InstructionBase(OpCode::CALL) {}
    void Dump(std::ostream &out = std::cout) override;
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
    void Dump(std::ostream &out = std::cout) override;
};

// Memory Instructions
class AllocInstruction : public BinaryInstruction {
  public:
    AllocInstruction() : BinaryInstruction(OpCode::ALLOC) {}
    void Dump(std::ostream &out = std::cout) override;
};

class FreeInstruction : public UnaryInstruction {
  public:
    FreeInstruction() : UnaryInstruction(OpCode::FREE) {}
    void Dump(std::ostream &out = std::cout) override;
};

class LoadInstruction : public BinaryInstruction {
  public:
    LoadInstruction() : BinaryInstruction(OpCode::LOAD) {}
    void Dump(std::ostream &out = std::cout) override;
};

class StoreInstruction : public BinaryInstruction {
  public:
    StoreInstruction() : BinaryInstruction(OpCode::STORE) {}
    void Dump(std::ostream &out = std::cout) override;
};

class PtraddInstruction : public TernaryInstruction {
  public:
    PtraddInstruction() : TernaryInstruction(OpCode::PTRADD) {}
    void Dump(std::ostream &out = std::cout) override;
};

// Miscellaneous Instructions
class IdInstruction final : public BinaryInstruction {
  public:
    IdInstruction() : BinaryInstruction(OpCode::ID) {}
    void Dump(std::ostream &out = std::cout) override;
};

class ConstInstruction final : public BinaryInstruction {
    // Src is ImmedOperand
  public:
    ConstInstruction() : BinaryInstruction(OpCode::CONST) {}
    void Dump(std::ostream &out = std::cout) override;
};

class PrintInstruction final : public InstructionBase {
  public:
    PrintInstruction() : InstructionBase(OpCode::PRINT) {}
    void Dump(std::ostream &out = std::cout) override;
};

class NopInstruction final : public InstructionBase {
  public:
    NopInstruction() : InstructionBase(OpCode::NOP) {}
    void Dump(std::ostream &out = std::cout) override;
};

class LabelInstruction final : public UnaryInstruction {
  public:
    LabelInstruction() : UnaryInstruction(OpCode::LABEL) {}
    void Dump(std::ostream &out = std::cout) override;
};

// Floating-Point Arithmetic Instructions
class FAddInstruction final : public TernaryInstruction {
  public:
    FAddInstruction() : TernaryInstruction(OpCode::FADD) {}
    void Dump(std::ostream &out = std::cout) override;
};

class FMulInstruction final : public TernaryInstruction {
  public:
    FMulInstruction() : TernaryInstruction(OpCode::FMUL) {}
    void Dump(std::ostream &out = std::cout) override;
};

class FSubInstruction final : public TernaryInstruction {
  public:
    FSubInstruction() : TernaryInstruction(OpCode::FSUB) {}
    void Dump(std::ostream &out = std::cout) override;
};

class FDivInstruction final : public TernaryInstruction {
  public:
    FDivInstruction() : TernaryInstruction(OpCode::FDIV) {}
    void Dump(std::ostream &out = std::cout) override;
};

// Floating-Pointi Comparison Instructions
class FEqInstruction final : public TernaryInstruction {
  public:
    FEqInstruction() : TernaryInstruction(OpCode::FEQ) {}
    void Dump(std::ostream &out = std::cout) override;
};

class FLtInstruction final : public TernaryInstruction {
  public:
    FLtInstruction() : TernaryInstruction(OpCode::FLT) {}
    void Dump(std::ostream &out = std::cout) override;
};

class FGtInstruction final : public TernaryInstruction {
  public:
    FGtInstruction() : TernaryInstruction(OpCode::FGT) {}
    void Dump(std::ostream &out = std::cout) override;
};

class FLeInstruction final : public TernaryInstruction {
  public:
    FLeInstruction() : TernaryInstruction(OpCode::FLE) {}
    void Dump(std::ostream &out = std::cout) override;
};

class FGeInstruction final : public TernaryInstruction {
  public:
    FGeInstruction() : TernaryInstruction(OpCode::FGE) {}
    void Dump(std::ostream &out = std::cout) override;
};
} // namespace sc
