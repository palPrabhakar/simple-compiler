#pragma once
#include "opcodes.hpp"
#include "operands.hpp"
#include <cassert>
#include <memory>
#include <vector>

namespace sc {

OpCode GetOpCodeFromStr(std::string);

class InstructionBase {
  public:
    virtual ~InstructionBase() = default;
    virtual void SetOperand(std::weak_ptr<OperandBase> oprnd, size_t idx) {
        if (idx < operands.size()) {
            operands[idx] = std::move(oprnd);
        } else {
            operands.push_back(std::move(oprnd));
        }
    }

    OpCode GetOpCode() const { return opcode; }

  protected:
    InstructionBase(OpCode _opcode) : opcode(_opcode) {}
    std::vector<std::weak_ptr<OperandBase>> operands;

  private:
    OpCode opcode;
};

class UnaryInstruction : public InstructionBase {
  public:
    UnaryInstruction(OpCode opcode) : InstructionBase(opcode) {}

    virtual void SetOperand(std::weak_ptr<OperandBase> oprnd,
                            size_t idx) override {
        assert(idx == 0 && "UnaryInstruction::SetOperand invalid index\n");
        InstructionBase::SetOperand(std::move(oprnd), 0);
    }
};

class BinaryInstruction : public InstructionBase {
  public:
    BinaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

    virtual void SetOperand(std::weak_ptr<OperandBase> oprnd,
                            size_t idx) override {
        assert((idx == 0 || idx == 1) &&
               "BinaryInstruction::SetOperand invalid index\n");
        InstructionBase::SetOperand(std::move(oprnd), idx);
    }
};

class TernaryInstruction : public InstructionBase {
  public:
    TernaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

    virtual void SetOperand(std::weak_ptr<OperandBase> oprnd,
                            size_t idx) override {
        assert((idx == 0 || idx == 1 || idx == 2) &&
               "BinaryInstruction::SetOperand invalid index\n");
        InstructionBase::SetOperand(std::move(oprnd), idx);
    }
};

// Arithmetic Instructions
class AddInstruction : public TernaryInstruction {
  public:
    AddInstruction() : TernaryInstruction(OpCode::ADD) {}
};

class MulInstruction : public TernaryInstruction {
  public:
    MulInstruction() : TernaryInstruction(OpCode::MUL) {}
};

class SubInstruction : public TernaryInstruction {
  public:
    SubInstruction() : TernaryInstruction(OpCode::SUB) {}
};

class DivInstruction : public TernaryInstruction {
  public:
    DivInstruction() : TernaryInstruction(OpCode::DIV) {}
};

// Comparison Instructions
class EqInstruction : public TernaryInstruction {
  public:
    EqInstruction() : TernaryInstruction(OpCode::EQ) {}
};

class LtInstruction : public TernaryInstruction {
  public:
    LtInstruction() : TernaryInstruction(OpCode::LT) {}
};

class GtInstruction : public TernaryInstruction {
  public:
    GtInstruction() : TernaryInstruction(OpCode::GT) {}
};

class LeInstruction : public TernaryInstruction {
  public:
    LeInstruction() : TernaryInstruction(OpCode::LE) {}
};

class GeInstruction : public TernaryInstruction {
  public:
    GeInstruction() : TernaryInstruction(OpCode::GE) {}
};

// Logic Instructions
class AndInstruction : public TernaryInstruction {
  public:
    AndInstruction() : TernaryInstruction(OpCode::AND) {}
};

class OrInstruction : public TernaryInstruction {
  public:
    OrInstruction() : TernaryInstruction(OpCode::OR) {}
};

class NotInstruction : public BinaryInstruction {
  public:
    NotInstruction() : BinaryInstruction(OpCode::NOT) {}
};

// Control Instruction
class JmpInstruction : public UnaryInstruction {
    public:
        JmpInstruction() : UnaryInstruction(OpCode::JMP) {}
};

class BranchInstruction : public TernaryInstruction {
    // One source and two destinations
    // Source - RegOperand
    // Dest - LabelOperand
  public:
    BranchInstruction() : TernaryInstruction(OpCode::BR) {}
};

class CallInstruction : public InstructionBase {
    // First operand is dest
    // Rest are arguments
  public:
    CallInstruction() : InstructionBase(OpCode::CALL) {}

    void SetFuncName(std::string fname) { func = fname; }

    std::string GetFuncName() const { return func; }

  private:
    std::string func;
};

class RetInstruction : public UnaryInstruction {
  public:
    RetInstruction() : UnaryInstruction(OpCode::RET) {}
};

// Miscellaneous Instructions
class IdInstruction : public BinaryInstruction {
  public:
    IdInstruction() : BinaryInstruction(OpCode::ID) {}
};

class ConstInstruction : public BinaryInstruction {
    // Src is ImmedOperand
  public:
    ConstInstruction() : BinaryInstruction(OpCode::CONST) {}
};

class PrintInstruction : public InstructionBase {
  public:
    PrintInstruction() : InstructionBase(OpCode::PRINT) {}
};

class NopInstruction : public InstructionBase {
  public:
    NopInstruction() : InstructionBase(OpCode::NOP) {}
};

class LabelInstruction : public UnaryInstruction {
  public:
    LabelInstruction() : UnaryInstruction(OpCode::LABEL) {}
};

} // namespace sc
