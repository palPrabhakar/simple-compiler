#pragma once

#include "block.hpp"
#include "operand.hpp"
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

    void AddArgs(OperandBase *operand) { args.push_back(std::move(operand)); }

    void AddOperand(std::unique_ptr<OperandBase> operand) {
        operands.push_back(std::move(operand));
    }

    void AddBlock(std::unique_ptr<Block> block) {
        blocks.push_back(std::move(block));
    }

    std::string GetName() const { return name; }

    OperandBase *GetOperand(size_t idx) { return operands[idx].get(); }

    OperandBase *GetArgs(size_t idx) { return args[idx]; }

    size_t GetBlockSize() { return blocks.size(); }

    size_t GetOperandSize() { return operands.size(); }

    size_t GetArgsSize() { return args.size(); }

    // Non-owning pointer
    Block *GetBlock(size_t idx) {
        assert(idx < blocks.size() && "Invalid index Function::GetBlock\n");
        return blocks[idx].get();
    }

    void Dump(std::ostream &out = std::cout);
    virtual std::string GetStrRetType() const;
    void BuildCFG();
    void DumpBlocks(std::ostream &out = std::cout);
    void DumpCFG(std::ostream &out = std::cout);

  protected:
    std::vector<std::unique_ptr<Block>> blocks;
    std::vector<std::unique_ptr<OperandBase>> operands;
    std::vector<OperandBase *> args;
    std::string name;
    DataType ret_type;

  private:
    void FixIR();
    void AddUniqueExitBlock(std::vector<Block *> rb);
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
