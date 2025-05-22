#pragma once

#include "block.hpp"
#include "operand.hpp"
#include "util.hpp"
#include <iostream>
#include <memory>
#include <ranges>
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
    virtual ~Function() = default;

    void AddArgs(OperandBase *operand) { args.push_back(std::move(operand)); }

    void AddOperand(std::unique_ptr<OperandBase> operand) {
        operands.push_back(std::move(operand));
    }

    void AddBlock(std::unique_ptr<Block> block) {
        blocks.push_back(std::move(block));
    }

    std::string GetName() const { return name; }

    DataType GetRetType() const { return ret_type; }

    size_t GetOperandSize() const { return operands.size(); }
    OperandBase *GetOperand(size_t idx) const { return operands[idx].get(); }

    size_t GetArgsSize() const { return args.size(); }
    OperandBase *GetArgs(size_t idx) const { return args[idx]; }
    std::span<OperandBase *> GetArgs() { return std::span(args); }

    size_t GetBlockSize() const { return blocks.size(); }

    // Non-owning pointer
    Block *GetBlock(size_t idx) {
        assert(idx < blocks.size());
        return blocks[idx].get();
    }

    auto GetBlocks() {
        // the underlying data managed by this class
        // can change via call to this func
        return blocks |
               std::views::transform([](auto &blk) { return blk.get(); });
    }

    void RemoveBlock(size_t idx) {
        // Not efficient avoid using.
        // Use helper function in utils
        assert(idx < blocks.size());
        blocks.erase(blocks.begin() + static_cast<long>(idx));

        for (auto i : std::views::iota(idx, blocks.size())) {
            blocks[i]->SetIndex(i);
        }
    }

    void RemoveBlocks(std::vector<size_t> indexes) {
        RemoveElements(blocks, std::move(indexes));

        size_t i = 0;
        for (auto &blk : blocks) {
            blk->SetIndex(i++);
        }
    }

    virtual std::string GetStrRetType() const;
    void Dump(std::ostream &out = std::cout) const;
    void DumpBlocks(std::ostream &out = std::cout) const;
    void DumpCFG(std::ostream &out = std::cout) const;
    void DumpDefUseLinks(std::ostream &out = std::cout) const;

  protected:
    std::vector<std::unique_ptr<Block>> blocks;
    std::vector<std::unique_ptr<OperandBase>> operands;
    // TODO:
    // Create internal instr
    std::vector<OperandBase *> args;
    std::string name;
    DataType ret_type;
};

class PtrFunction final : public Function {
  public:
    PtrFunction(std::string name) : Function(name, DataType::PTR) {}
    void AppendPtrChain(DataType type) { ptr_chain.push_back(type); }
    const std::vector<DataType> &GetPtrChain() const { return ptr_chain; }
    virtual std::string GetStrRetType() const override;

  private:
    std::vector<DataType> ptr_chain;
};
} // namespace sc
