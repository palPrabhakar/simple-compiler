#pragma once

#include "analyzer.hpp"
#include "instruction.hpp"
#include "program.hpp"
#include <cassert>
#include <memory>
#include <ranges>
#include <stack>
#include <string>
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
    // TODO:
    // Build def-uses links
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

class DVNTransformer final : public Transformer {
    // Dominator-based Value Numbering
  public:
    DVNTransformer(Function *_f)
        : Transformer(_f), dom(_f), remove_instrs(_f->GetBlockSize()) {}

    void Transform() override {
        if (func->GetArgsSize()) {
            vt.PushScope();
            for (auto i : std::views::iota(0ul, func->GetArgsSize())) {
                auto *arg = func->GetArgs(i);
                vt.Insert(arg->GetName(), arg);
            }
        }

        dom.BuildDominatorTree();
        DVN(func->GetBlock(0));
        RemoveInstructions();
    }

  private:
    DominatorAnalyzer dom;
    std::vector<std::vector<size_t>> remove_instrs;

    class ScopedVTable {
      public:
        void PushScope() { st.push_back({}); }

        void Insert(const std::string &key, OperandBase *op) {
            assert(!st.empty() && "Empty stack!\n");
            st.back()[key] = op;
        }

        OperandBase *Get(const std::string &key, bool local = false) const {
            auto it = st.rbegin();
            do {
                if(it->contains(key)) {
                    return it->at(key);
                }
            } while (!local && ++it != st.rend());

            return nullptr;
        }

        void PopScope() {
            // Let's keep it simple
            st.erase(st.end() - 1);
        }

      private:
        std::vector<std::unordered_map<std::string, OperandBase *>> st;
    } vt; // value table

    bool IsUseless(GetInstruction *geti);
    bool IsRedundant(GetInstruction *geti);
    void DVN(Block *block);
    void RemoveInstructions();

    std::string GetKey(InstructionBase *instr) const {
        return instr->GetOperand(1)->GetName() +
               std::to_string(static_cast<int>(instr->GetOpCode())) +
               instr->GetOperand(2)->GetName();
    }
};
} // namespace sc
