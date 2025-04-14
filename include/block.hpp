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

    InstructionBase *GetInstruction(size_t idx) {
        assert(idx < instructions.size() &&
               "Invalid index Block::GetInstruction\n");
        return instructions[idx].get();
    }

    std::string GetBlockName() const { return name; }

    size_t GetInstructionSize() { return instructions.size(); }

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
    std::string name;
};
} // namespace sc
