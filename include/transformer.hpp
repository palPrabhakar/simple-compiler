#pragma once

#include "cfg.hpp"
#include "program.hpp"
#include <cassert>
#include <memory>
#include <ranges>

namespace sc {
class Transformer {
  public:
    virtual ~Transformer() = default;
    virtual std::unique_ptr<Program>
    Transform(std::unique_ptr<Program> program) = 0;

  protected:
    Transformer() = default;
};

class EarlyIRTransformer : public Transformer {
  public:
    EarlyIRTransformer() {}
    std::unique_ptr<Program>
    Transform(std::unique_ptr<Program> program) override {
        for (auto &f : *program) {
            FixIR(f.get());
        }
        return program;
    }

  private:
    void FixIR(Function *func);
    void AddUniqueExitBlock(std::vector<Block *> rb, Function *func);
};

class CFTransformer : public Transformer {
  public:
    std::unique_ptr<Program>
    Transform(std::unique_ptr<Program> program) override;

  private:
    std::vector<Block *> traverse_order;

    void RemoveUnreachableCFNode(Function *func);
    bool Clean();
    void ReplaceBrWithJmp(Block *block);
    void RemoveEmptyBlock(Block *block);
    void CombineBlocks(Block *block);
    void HoistBranch(Block *block);

    void ReplacePredecessor(Block *block, Block *new_blk, Block *old_blk) {
        for (size_t i : std::views::iota(0UL, block->GetPredecessorSize())) {
            if (block->GetPredecessor(i) == old_blk) {
                block->AddPredecessor(new_blk, i);
                return;
            }
        }
    }

    void RemovePredecessorIf(Block *block, Block *match) {
        for (size_t i : std::views::iota(0UL, block->GetPredecessorSize())) {
            if (block->GetPredecessor(i) == match) {
                block->RemovePredecessor(i);
                return;
            }
        }
    }

    void RemoveSuccessorIf(Block *block, Block *match) {
        for (size_t i : std::views::iota(0UL, block->GetSuccessorSize())) {
            if (block->GetSuccessor(i) == match) {
                block->RemoveSuccessor(i);
                return;
            }
        }
    }
};

} // namespace sc
