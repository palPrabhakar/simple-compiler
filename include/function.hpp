#pragma once

#include "instructions.hpp"
#include "operands.hpp"
#include <memory>
#include <string>
#include <vector>

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

    std::string GetName() const { return name; }

  private:
    std::vector<std::unique_ptr<InstructionBase>> instructions;
    std::vector<std::shared_ptr<OperandBase>> operands;
    std::vector<std::weak_ptr<OperandBase>> args;
    std::string name;
};
} // namespace sc
