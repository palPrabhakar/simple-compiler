#pragma once

#include "transformer.hpp"
#include "analyzers/dominator_analyzer.hpp"
#include "instruction.hpp"
#include <vector>
#include <string>
#include <ranges>
#include <unordered_map>

namespace sc {

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

        dom.BuildRPODominatorTree();
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
                if (it->contains(key)) {
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


    bool IsUselessOrRedundant(GetInstruction *geti, std::string &key);
    void DVN(Block *block);
    void RemoveInstructions();

    std::string GetKey(InstructionBase *instr) const {
        return instr->GetOperand(1)->GetName() +
               std::to_string(static_cast<int>(instr->GetOpCode())) +
               instr->GetOperand(2)->GetName();
    }
};
} // namespace sc
