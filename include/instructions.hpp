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

class ConstInstruction : public BinaryInstruction {
    // Src is ImmedOperand
  public:
    ConstInstruction() : BinaryInstruction(OpCode::CONST) {}
};

class ArithmeticInstruction : public TernaryInstruction {
  public:
    ArithmeticInstruction(OpCode _opcode) : TernaryInstruction(_opcode) {
        assert((_opcode >= OpCode::ADD && _opcode <= OpCode::GE) &&
               "OpCode not for ArithmeticInstruction\n");
    }
};

class PrintInstruction : public InstructionBase {
  public:
    PrintInstruction() : InstructionBase(OpCode::PRINT) {}
};

class BranchInstruction : public TernaryInstruction {
    // One source and two destinations
    // Source - RegOperand
    // Dest - LabelOperand
  public:
    BranchInstruction() : TernaryInstruction(OpCode::BR) {}
};

class LabelInstruction : public UnaryInstruction {
  public:
    LabelInstruction() : UnaryInstruction(OpCode::LABEL) {}
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
} // namespace sc
