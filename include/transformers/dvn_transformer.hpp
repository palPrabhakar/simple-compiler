#pragma once

#include "analyzers/dominator_analyzer.hpp"
#include "instruction.hpp"
#include "transformer.hpp"
#include "transformers/interpreter.hpp"
#include "transformers/expression_simplifier.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace sc {

/*
 * Dominator-based value numbering with const propagation, const folding,
 * expression simplification. See Cooper and Torczon Ch 8.5.
 */
class DVNTransformer final : public Transformer {
    // Dominator-based Value Numbering
  public:
    DVNTransformer(Function *_f)
        : Transformer(_f), dom(_f), remove_instrs(_f->GetBlockSize()) {}

    void Transform() override;

  private:
    DominatorAnalyzer dom;
    Interpreter interpreter;
    ExpressionSimplifier simplifier;
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

    void DVN(Block *block);

    void Process(InstructionBase *instr, size_t idx);

    bool IsUselessOrRedundant(GetInstruction *geti, std::string &key);

    void MarkForRemoval(Block *block, size_t idx);

    InstructionBase *FoldConstInstruction(InstructionBase *instr, size_t idx);

    std::pair<std::string, OperandBase *>
    GetKeyAndVN(InstructionBase *instr) const;

    void RemoveInstructions();
};
} // namespace sc
