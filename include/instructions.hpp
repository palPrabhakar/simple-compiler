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

  protected:
    InstructionBase(OpCode _op_code) : op_code(_op_code) {}

  private:
    OpCode op_code;
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
    ConstInstruction(OpCode _op_code = OpCode::CONST)
        : BinaryInstruction(_op_code) {
        assert(_op_code == OpCode::CONST &&
               "OpCode not CONST for ConstInstruction\n");
    }
};

class BranchInstruction : public TernaryInstruction {
    // One source and two destinations
};

class RetInstruction : public UnaryInstruction {};

class CallInstruction : public InstructionBase {};

class PrintInstruction : public InstructionBase {};

std::unique_ptr<InstructionBase> MakeConstInstruction(sjp::Json &instr); } // namespace sc
