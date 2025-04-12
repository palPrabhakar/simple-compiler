#pragma once

#include "instructions.hpp"
#include "operands.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace sc {
class Function {
  public:
    Function(std::string _name, DataType _ret_type)
        : name(_name), ret_type(_ret_type) {}
    ~Function() = default;

    void AddOperand(std::unique_ptr<OperandBase> operand) {
        operands.push_back(std::move(operand));
    }
    void AddArgs(OperandBase *operand) { args.push_back(std::move(operand)); }
    void AddInstructions(std::unique_ptr<InstructionBase> instr) {
        instructions.push_back(std::move(instr));
    }

    void Dump(std::ostream &out = std::cout);

    std::string GetName() const { return name; }
    OperandBase *GetOperand(size_t idx) { return operands[idx].get(); }
    OperandBase *GetArgs(size_t idx) { return args[idx]; }
    size_t GetInstructionSize() { return instructions.size(); }
    size_t GetOperandSize() { return operands.size(); }
    size_t GetArgsSize() { return args.size(); }
    InstructionBase *GetInstruction(size_t idx) {
        // non-owning pointer
        return instructions[idx].get();
    }
    virtual std::string GetStrRetType() const;

  protected:
    std::vector<std::unique_ptr<InstructionBase>> instructions;
    std::vector<std::unique_ptr<OperandBase>> operands;
    std::vector<OperandBase *> args;
    std::string name;
    DataType ret_type;
};

class PtrFunction : public Function {
  public:
    PtrFunction(std::string name) : Function(name, DataType::PTR) {}
    void AppendPtrChain(DataType type) { ptr_chain.push_back(type); }
    const std::vector<DataType> &GetPtrChain() const { return ptr_chain; }
    virtual std::string GetStrRetType() const override;

  private:
    std::vector<DataType> ptr_chain;
};

} // namespace sc
