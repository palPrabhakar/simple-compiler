#pragma once

#include "block.hpp"
#include "operand.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace sc {

#define LAST_BLK(func) func->GetBlock(func->GetBlockSize() - 1)
#define APPEND_INSTR(fun, instr)                                               \
    LAST_BLK(func)->AddInstruction(std::move(instr))

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

    DataType GetRetType() const { return ret_type; }

    OperandBase *GetOperand(size_t idx) { return operands[idx].get(); }

    OperandBase *GetArgs(size_t idx) { return args[idx]; }

    size_t GetBlockSize() const { return blocks.size(); }

    size_t GetOperandSize() const { return operands.size(); }

    size_t GetArgsSize() { return args.size(); }

    // Non-owning pointer
    Block *GetBlock(size_t idx) {
        assert(idx < blocks.size() && "Invalid index Function::GetBlock\n");
        return blocks[idx].get();
    }

    std::vector<std::unique_ptr<Block>> &GetBlocks() { return blocks; }

    virtual std::string GetStrRetType() const;
    void Dump(std::ostream &out = std::cout);
    void DumpBlocks(std::ostream &out = std::cout);
    void DumpCFG(std::ostream &out = std::cout);

  protected:
    std::vector<std::unique_ptr<Block>> blocks;
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
