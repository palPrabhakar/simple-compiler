#include "analyzers/globals_analyzer.hpp"
#include "instruction.hpp"
#include "operand.hpp"
#include <iostream>
#include <ranges>
#include <unordered_set>

namespace sc {
// GlobalsAnalyzer begin
void GlobalsAnalyzer::ComputeGlobalNames() {
    auto var_kill = std::unordered_set<OperandBase *>();

    auto *block = func->GetBlock(0);
    for (auto *arg : func->GetArgs()) {
        var_kill.insert(arg);
        blocks[arg].insert(block);
    }

    for (auto *block : func->GetBlocks()) {
        for (auto *instr : block->GetInstructions()) {
            auto Opcode = instr->GetOpcode();
            assert(Opcode != Opcode::GET && Opcode != Opcode::SET);
            Process(var_kill, instr, block);
        }
        // Clear the var_kill set before the start of next iteration!
        // Can't clear above becuase in case of first block the var_kill
        // set will have function arguments.
        var_kill.clear();
    }
}

void GlobalsAnalyzer::DumpGlobals(std::ostream &out) const {
    out << "Globals: " << func->GetName() << "\n";
    for (auto *op : globals) {
        out << "  " << op->GetName() << "\n";
    }
}

void GlobalsAnalyzer::DumpBlocks(std::ostream &out) const {
    out << "Blocks " << func->GetName() << "\n";
    for (auto &[k, v] : blocks) {
        out << "  Operand: " << k->GetName() << "\n";
        out << "  ";
        for (auto *b : v) {
            out << "  " << b->GetName();
        }
        out << "\n";
    }
}

void GlobalsAnalyzer::Process(std::unordered_set<OperandBase *> &var_kill,
                              InstructionBase *instr, Block *block) {
    for (auto *op : instr->GetOperands()) {
        if (!var_kill.contains(op)) {
            globals.insert(op);
        }
    }

    if (instr->HasDest()) {
        var_kill.insert(instr->GetDest());
        blocks[instr->GetDest()].insert(block);
    }
}
// GlobalsAnalyzer end
} // namespace sc
