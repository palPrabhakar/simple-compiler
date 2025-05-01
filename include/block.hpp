#pragma once

#include "instruction.hpp"
#include "operand.hpp"
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sc {

using instr_ptr = std::unique_ptr<InstructionBase>;

#define LAST_INSTR(block) block->GetInstruction(block->GetInstructionSize() - 1)

class Block {
  public:
    Block(std::string _name) : name(_name) {}

    void SetLabel(LabelOperand *lbl) { label = lbl; }

    void SetIndex(size_t _idx) { idx = _idx; }

    void AddInstruction(instr_ptr instr) {
        instructions.push_back(std::move(instr));
    }

    void AddInstruction(instr_ptr instr, size_t idx) {
        assert(idx < instructions.size());
        instructions[idx] = std::move(instr);
    }

    void InsertInstructions(std::vector<instr_ptr> instrs, size_t idx) {
        instructions.insert(instructions.begin() + idx,
                            std::make_move_iterator(instrs.begin()),
                            std::make_move_iterator(instrs.end()));
    }

    void InsertInstruction(instr_ptr instr, size_t idx) {
        instructions.insert(instructions.begin() + idx, std::move(instr));
    }

    void RemoveInstruction(size_t idx) {
        assert(idx < instructions.size());
        instructions.erase(instructions.begin() + idx);
    }

    std::vector<instr_ptr> ReleaseInstructions() {
        return std::move(instructions);
    }

    void AddSuccessor(Block *succ) { successors.push_back(succ); }

    void AddSuccessor(Block *succ, size_t idx) {
        assert(idx < successors.size());
        successors[idx] = succ;
    }

    void RemoveSuccessor(size_t idx) {
        assert(idx < successors.size());
        successors.erase(successors.begin() + idx);
    }

    void AddPredecessor(Block *pred) { predecessors.push_back(pred); }

    void AddPredecessor(Block *pred, size_t idx) {
        assert(idx < predecessors.size());
        predecessors[idx] = pred;
    }

    void RemovePredecessor(size_t idx) {
        assert(idx < predecessors.size());
        predecessors.erase(predecessors.begin() + idx);
    }

    InstructionBase *GetInstruction(size_t idx) {
        assert(idx < instructions.size());
        return instructions[idx].get();
    }

    std::string GetName() const { return name; }

    size_t GetIndex() const { return idx; }

    LabelOperand *GetLabel() const { return label; }

    size_t GetInstructionSize() const { return instructions.size(); }

    size_t GetSuccessorSize() const { return successors.size(); }

    size_t GetPredecessorSize() const { return predecessors.size(); }

    Block *GetSuccessor(size_t idx) {
        assert(idx < successors.size() &&
               "Invalid index Block::GetSuccessor\n");
        return successors[idx];
    }

    const std::vector<Block *> &GetSuccessors() const { return successors; }

    const std::vector<Block *> &GetPredecessors() const { return predecessors; }

    Block *GetPredecessor(size_t idx) {
        assert(idx < predecessors.size() &&
               "Invalid index Block::GetPredecessor\n");
        return predecessors[idx];
    }

    void Dump(std::ostream &out = std::cout, std::string prefix = "") {
        for (auto &i : instructions) {
            i->Dump(out << prefix);
        }
    }

    std::vector<instr_ptr>::iterator begin() { return instructions.begin(); }

    std::vector<instr_ptr>::const_iterator begin() const {
        return instructions.begin();
    }

    std::vector<instr_ptr>::const_iterator cbegin() const {
        return instructions.cbegin();
    }

    std::vector<instr_ptr>::iterator end() { return instructions.end(); }

    std::vector<instr_ptr>::const_iterator end() const {
        return instructions.end();
    }

    std::vector<instr_ptr>::const_iterator cend() const {
        return instructions.cend();
    }

  private:
    std::vector<instr_ptr> instructions;
    std::vector<Block *> successors;   // cfg successors
    std::vector<Block *> predecessors; // cfg predecessors
    std::string name;
    size_t idx;
    LabelOperand *label = nullptr;
};
} // namespace sc
