#pragma once

#include "instructions.hpp"
#include "operands.hpp"
#include <memory>
#include <string>
#include <vector>
#include <iostream>

namespace sc {
class Function {
  public:
    Function(std::string _name) : name(_name) {}
    ~Function() = default;

    void AddOperand(std::shared_ptr<OperandBase> operand) {
        operands.push_back(std::move(operand));
    }
    void AddArgs(std::weak_ptr<OperandBase> operand) {
        args.push_back(std::move(operand));
    }
    void AddInstructions(std::unique_ptr<InstructionBase> instr) {
        instructions.push_back(std::move(instr));
    }

    std::string GetName() const { return name; }
    std::unique_ptr<InstructionBase> &GetInstruction(size_t idx) {
        return instructions[idx];
    }
    std::weak_ptr<OperandBase> GetOperand(size_t idx) { return operands[idx]; }
    std::weak_ptr<OperandBase> GetArgs(size_t idx) { return args[idx]; }
    size_t GetInstructionSize() { return instructions.size(); }
    size_t GetOperandSize() { return operands.size(); }
    size_t GetArgsSize() { return args.size(); }

  private:
    std::vector<std::unique_ptr<InstructionBase>> instructions;
    std::vector<std::shared_ptr<OperandBase>> operands;
    std::vector<std::weak_ptr<OperandBase>> args;
    std::string name;
};
} // namespace sc
