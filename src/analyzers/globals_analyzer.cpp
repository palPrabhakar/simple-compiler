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
    auto process = [&var_kill, &globals = this->globals,
                    &blocks = this->blocks](InstructionBase *instr,
                                            Block *block, size_t start,
                                            bool dest = true) {
        for (auto i : std::views::iota(start, instr->GetOperandSize())) {
            if (!var_kill.contains(instr->GetOperand(i))) {
                globals.insert(instr->GetOperand(i));
            }
        }

        if (dest) {
            var_kill.insert(instr->GetOperand(0));
            blocks[instr->GetOperand(0)].insert(block);
        }
    };

    if (func->GetArgsSize()) {
        auto *block = func->GetBlock(0);
        for (auto i : std::views::iota(0ul, func->GetArgsSize())) {
            var_kill.insert(func->GetArgs(i));
            blocks[func->GetArgs(i)].insert(block);
        }
    }

    for (auto b : std::views::iota(0ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(b);

        for (auto i : std::views::iota(0ul, block->GetInstructionSize())) {
            auto *instr = block->GetInstruction(i);
            auto opcode = instr->GetOpCode();
            switch (opcode) {
            case OpCode::JMP:
            case OpCode::LABEL:
            case OpCode::NOP:
                // Don't do anything
                break;
            case OpCode::SET:
            case OpCode::GET:
                assert(false && "Unexpected opcode\n");
            case OpCode::BR:
                process(instr, block, 2, false);
                break;
            case OpCode::CALL: {
                auto *call = static_cast<CallInstruction *>(instr);
                process(instr, block, call->GetRetVal(), call->GetRetVal());
            } break;
            case OpCode::RET: {
                auto *ret = static_cast<RetInstruction *>(instr);
                if (ret->GetOperandSize()) {
                    process(instr, block, 0, false);
                }
            } break;
            case OpCode::FREE:
            case OpCode::STORE:
            case OpCode::PRINT:
                process(instr, block, 0, false);
                break;
            case OpCode::CONST:
                process(instr, block, instr->GetOperandSize(), true);
                break;
            default:
                // Defines a value
                process(instr, block, 1, true);
                break;
            }
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
// GlobalsAnalyzer end
} // namespace sc
