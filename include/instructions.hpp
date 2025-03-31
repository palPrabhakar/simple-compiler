#pragma once
#include "OpCodes.hpp"
#include "operands.hpp"
#include <memory>

namespace sc {
class InstructionBase {
  public:
    virtual ~InstructionBase();

  protected:
    InstructionBase(OpCode _op_code) : op_code(_op_code) {}

  private:
    OpCode op_code;
};

class UnaryInstruction : public InstructionBase {
  public:
    UnaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

  private:
    std::weak_ptr<OperandBase> operands[2];
};

class BinaryInstruction : public InstructionBase {
  public:
    BinaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

  private:
    std::weak_ptr<OperandBase> operands[3];
};
} // namespace sc
