#pragma once

#include "program.hpp"
#include <memory>
#include <vector>

namespace sc {

class CFGContainer {
  public:
    virtual bool HasSuccessors(Block *) = 0;
    virtual std::span<Block *> GetSuccessors(Block *) = 0;

    virtual bool HasPredecessors(Block *) = 0;
    virtual std::span<Block *> GetPredecessors(Block *) = 0;

    Block *GetRoot() const { return root; }

  protected:
    CFGContainer(Block *blk) : root(blk) {}

    Block *root;
};

class ForwardCFG : public CFGContainer {
  public:
    ForwardCFG(Function *func) : CFGContainer(func->GetBlock(0)) {}

    bool HasSuccessors(Block *blk) override {
        return blk->GetSuccessorSize();
    }

    std::span<Block *> GetSuccessors(Block *blk) override {
        return blk->GetSuccessors();
    }

    bool HasPredecessors(Block *blk) override {
        return blk->GetPredecessorSize();
    }

    std::span<Block *> GetPredecessors(Block *blk) override {
        return blk->GetPredecessors();
    }
};

class ReverseCFG : public CFGContainer {
  public:
    ReverseCFG(Function *func) : CFGContainer(LAST_BLK(func)) {}

    bool HasSuccessors(Block *blk) override {
        return blk->GetPredecessorSize();
    }

    std::span<Block *> GetSuccessors(Block *blk) override {
        return blk->GetPredecessors();
    }

    bool HasPredecessors(Block *blk) override {
        return blk->GetSuccessorSize();
    }

    std::span<Block *> GetPredecessors(Block *blk) override {
        return blk->GetSuccessors();
    }
};

using CFG = CFGContainer;

const std::vector<Block *> GetPostOrder(CFG *cfg);
const std::vector<Block *> GetReversePostOrder(CFG *cfg);
std::unique_ptr<Program> BuildCFG(std::unique_ptr<Program> program);
}; // namespace sc
