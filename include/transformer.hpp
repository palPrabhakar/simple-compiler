#pragma once

#include "analyzer.hpp"
#include "program.hpp"
#include <cassert>
#include <memory>
#include <ranges>
#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace sc {

template <typename T> using stack = std::stack<T, std::vector<T>>;

template <typename TT>
std::unique_ptr<Program> ApplyTransformation(std::unique_ptr<Program> program) {
    for (auto &f : *program) {
        TT t(f.get());
        t.Transform();
    }
    return program;
}

class Transformer {
  public:
    virtual ~Transformer() = default;
    virtual void Transform() = 0;

  protected:
    Function *func;

    Transformer(Function *f) : func(f) {}
};

class EarlyIRTransformer final : public Transformer {
  public:
    EarlyIRTransformer(Function *f) : Transformer(f) {}
    void Transform() override;

  private:
    void FixIR();
    void AddUniqueExitBlock(std::vector<Block *> rb);
};

class CFTransformer final : public Transformer {
  public:
    CFTransformer(Function *f) : Transformer(f) {}

    void Transform() override;

  private:
    std::vector<Block *> traverse_order;

    void RemoveUnreachableCFNode();
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

class SSATransformer final : public Transformer {
  public:
    SSATransformer(Function *_f)
        : Transformer(_f), dom(_f), globals(_f), gets(_f->GetBlockSize()) {}

    void Transform() override { RewriteInSSAForm(); }

  private:
    DominatorAnalyzer dom;
    GlobalsAnalyzer globals;
    std::vector<std::unordered_set<OperandBase *>> gets;
    std::unordered_map<OperandBase *, size_t> counter;
    std::unordered_map<OperandBase *, sc::stack<OperandBase *>> name;

    void RewriteInSSAForm();
    void RenameGet(Block *block);

    OperandBase *NewDest(OperandBase *op);
};

} // namespace sc
