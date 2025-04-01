#pragma once
#include "json.hpp"
#include "opcodes.hpp"
#include "operands.hpp"
#include <cassert>
#include <memory>

namespace sc {

OpCode GetOpCodeFromStr(std::string);

class InstructionBase {
  public:
    virtual ~InstructionBase() = default;
    virtual void SetOperand(std::weak_ptr<OperandBase>, size_t) = 0;

    OpCode GetOpCode() const { return opcode; }

  protected:
    InstructionBase(OpCode _opcode) : opcode(_opcode) {}

  private:
    OpCode opcode;
};

class UnaryInstruction : public InstructionBase {
  public:
    UnaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

    virtual void SetOperand(std::weak_ptr<OperandBase> _operand,
                            size_t idx) override {
        assert(idx == 0 && "UnaryInstruction::SetOperand invalid index\n");
        operand = _operand;
    }

  private:
    std::weak_ptr<OperandBase> operand;
};

class BinaryInstruction : public InstructionBase {
  public:
    BinaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

    virtual void SetOperand(std::weak_ptr<OperandBase> _operand,
                            size_t idx) override {
        assert((idx == 0 || idx == 1) &&
               "BinaryInstruction::SetOperand invalid index\n");
        operands[idx] = _operand;
    }

  private:
    std::weak_ptr<OperandBase> operands[2];
};

class TernaryInstruction : public InstructionBase {
  public:
    TernaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

    virtual void SetOperand(std::weak_ptr<OperandBase> _operand,
                            size_t idx) override {
        assert((idx == 0 || idx == 1 || idx == 2) &&
               "BinaryInstruction::SetOperand invalid index\n");
        operands[idx] = _operand;
    }

  private:
    std::weak_ptr<OperandBase> operands[3];
};

class ConstInstruction : public BinaryInstruction {
    // Src is ImmedOperand
  public:
    ConstInstruction() : BinaryInstruction(OpCode::CONST) {}
};

class ArithmeticInstruction : public TernaryInstruction {
  public:
    ArithmeticInstruction(OpCode _opcode) : TernaryInstruction(_opcode) {
        assert((_opcode >= OpCode::ADD && _opcode <= OpCode::DIV) &&
               "OpCode not for ArithmeticInstruction\n");
    }
};

class PrintInstruction : public InstructionBase {
  public:
    PrintInstruction() : InstructionBase(OpCode::PRINT) {}

    void SetOperand(std::weak_ptr<OperandBase> oprnd, size_t idx) {
        if (idx < operands.size()) {
            operands[idx] = std::move(oprnd);
        } else {
            operands.push_back(std::move(oprnd));
        }
    }

  private:
    std::vector<std::weak_ptr<OperandBase>> operands;
};

class BranchInstruction : public TernaryInstruction {
    // One source and two destinations
};

class RetInstruction : public UnaryInstruction {};

class CallInstruction : public InstructionBase {};

std::unique_ptr<InstructionBase> MakeConstInstruction(sjp::Json &instr);
} // namespace sc
