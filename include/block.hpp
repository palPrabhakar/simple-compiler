#pragma once

#include "instruction.hpp"
#include <memory>
#include <string>
#include <vector>

namespace sc {

using instr_ptr = std::unique_ptr<InstructionBase>;

class Block {
  public:
    Block(std::string _name) : name(_name) {}

    void AddInstruction(instr_ptr instr) {
        instructions.push_back(std::move(instr));
    }

    void AddInstruction(instr_ptr instr, size_t idx) {
        assert(idx < instructions.size() &&
               "idx out of bounds Block::AddInstruction\n");
        instructions[idx] = std::move(instr);
    }

    void AddSuccessor(Block *succ) { cfg_succ.push_back(succ); }

    InstructionBase *GetInstruction(size_t idx) {
        assert(idx < instructions.size() &&
               "Invalid index Block::GetInstruction\n");
        return instructions[idx].get();
    }

    std::string GetName() const { return name; }

    size_t GetInstructionSize() { return instructions.size(); }

    size_t GetSuccessorSize() { return cfg_succ.size(); }

    const std::vector<Block *> &GetSuccessors() const { return cfg_succ; }

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
    std::vector<Block *> cfg_succ; // cfg successors
    std::string name;
};
} // namespace sc
