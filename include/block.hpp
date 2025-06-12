#pragma once

#include "instruction.hpp"
#include "operand.hpp"
#include "util.hpp"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace sc {

using instr_ptr = std::unique_ptr<InstructionBase>;

#define LAST_INSTR(block) block->GetInstruction(block->GetInstructionSize() - 1)

class Block {
  public:
    Block(std::string _name) : name(_name) {}
    Block(std::string _name, std::unique_ptr<LabelOperand> lbl)
        : name(_name), label(std::move(lbl)) {}

    /*
     * Name
     */
    std::string GetName() const { return name; }

    /*
     * Labels
     */
    void SetLabel(std::unique_ptr<LabelOperand> lbl) { label = std::move(lbl); }

    LabelOperand *GetLabel() const { return label.get(); }

    /*
     * Index
     */
    void SetIndex(size_t _idx) { idx = _idx; }

    size_t GetIndex() const { return idx; }

    /*
     * Instructions
     */
    void AddInstruction(instr_ptr instr) {
        instr->SetBlock(this);
        instr->SetIndex(instructions.size());
        instructions.push_back(std::move(instr));
    }

    void AddInstruction(instr_ptr instr, size_t idx, bool remove_from_usage = false) {
        /*
         * Replaces the instruction at idx
         */
        assert(idx < instructions.size());
        if (remove_from_usage) {
            for (auto *op : instructions[idx]->GetOperands()) {
                op->RemoveUse(instructions[idx].get());
            }
        }

        instr->SetBlock(this);
        instr->SetIndex(instructions.size());
        instructions[idx] = std::move(instr);
    }

    void InsertInstructions(std::vector<instr_ptr> instrs, size_t idx) {
        std::ranges::for_each(instrs, [this, i = idx](instr_ptr &ptr) mutable {
            ptr->SetBlock(this);
            ptr->SetIndex(i++);
        });

        instructions.insert(instructions.begin() + static_cast<long>(idx),
                            std::make_move_iterator(instrs.begin()),
                            std::make_move_iterator(instrs.end()));

        // Need to fix the index after this
        FixIndex(idx + instrs.size());
    }

    void InsertInstruction(instr_ptr instr, size_t idx) {
        /*
         * Doesn't replace the instruction at idx
         * Moves the instr at idx to idx + 1
         */
        instr->SetBlock(this);
        instr->SetIndex(idx);
        instructions.insert(instructions.begin() + static_cast<long>(idx),
                            std::move(instr));
        FixIndex(idx + 1);
    }

    size_t GetInstructionSize() const { return instructions.size(); }

    InstructionBase *GetInstruction(size_t idx) const {
        assert(idx < instructions.size());
        return instructions[idx].get();
    }

    auto GetInstructions() {
        return instructions |
               std::views::transform([](auto &i) { return i.get(); });
    }

    void RemoveInstruction(size_t idx, bool remove_from_usage = false) {
        assert(idx < instructions.size());
        if (remove_from_usage) {
            for (auto *op : instructions[idx]->GetOperands()) {
                op->RemoveUse(instructions[idx].get());
            }
        }

        instructions.erase(instructions.begin() + static_cast<long>(idx));
        FixIndex(idx);
    }

    void RemoveInstructions(std::vector<size_t> indexes,
                            bool remove_from_usage = false) {
        if (remove_from_usage) {
            for (auto idx : indexes) {
                for (auto *op : instructions[idx]->GetOperands()) {
                    op->RemoveUse(instructions[idx].get());
                }
            }
        }

        RemoveElements(instructions, std::move(indexes));
        FixIndex(0);
    }

    std::vector<instr_ptr> ReleaseInstructions() {
        std::ranges::for_each(instructions,
                              [](instr_ptr &ptr) { ptr->SetBlock(nullptr); });

        return std::move(instructions);
    }

    /*
     * Successors
     */
    void AddSuccessor(Block *succ) { successors.push_back(succ); }

    void AddSuccessor(Block *succ, size_t idx) {
        assert(idx < successors.size());
        successors[idx] = succ;
    }

    size_t GetSuccessorSize() const { return successors.size(); }

    Block *GetSuccessor(size_t idx) const {
        assert(idx < successors.size() &&
               "Invalid index Block::GetSuccessor\n");
        return successors[idx];
    }

    std::span<Block *> GetSuccessors() {
        return std::span<Block *>(successors);
    }

    void RemoveSuccessor(size_t idx) {
        assert(idx < successors.size());
        successors.erase(successors.begin() + static_cast<long>(idx));
    }

    void RemoveSuccessor(Block *block) {
        auto it = std::find(successors.begin(), successors.end(), block);
        if (it != successors.end()) {
            successors.erase(it);
        }
    }

    /*
     * Predecessors
     */
    void AddPredecessor(Block *pred) { predecessors.push_back(pred); }

    void AddPredecessor(Block *pred, size_t idx) {
        assert(idx < predecessors.size());
        predecessors[idx] = pred;
    }

    size_t GetPredecessorSize() const { return predecessors.size(); }

    Block *GetPredecessor(size_t idx) const {
        assert(idx < predecessors.size() &&
               "Invalid index Block::GetPredecessor\n");
        return predecessors[idx];
    }

    std::span<Block *> GetPredecessors() {
        return std::span<Block *>(predecessors);
    }

    void RemovePredecessor(size_t idx) {
        assert(idx < predecessors.size());
        predecessors.erase(predecessors.begin() + static_cast<long>(idx));
    }

    void RemovePredecessor(Block *block) {
        auto it = std::find(predecessors.begin(), predecessors.end(), block);
        if (it != predecessors.end()) {
            predecessors.erase(it);
        }
    }

    /*
     * Dump
     */
    void Dump(std::ostream &out = std::cout, std::string prefix = "") const {
        for (auto &i : instructions) {
            if (i->GetOpcode() != Opcode::GETARG) {
                i->Dump(out << prefix);
            }
        }
    }

  private:
    std::vector<instr_ptr> instructions;
    std::vector<Block *> successors;   // cfg successors
    std::vector<Block *> predecessors; // cfg predecessors
    std::string name;
    size_t idx;
    std::unique_ptr<LabelOperand> label = nullptr;

    void FixIndex(size_t idx) {
        for (auto i : std::views::iota(idx, instructions.size())) {
            instructions[i]->SetIndex(i);
        }
    }
};
} // namespace sc
