#include "analyzer.hpp"
#include "cfg.hpp"
#include "instruction.hpp"
#include "operand.hpp"
#include <iostream>
#include <ranges>
#include <unordered_set>

namespace sc {
// DominatorAnalyzer begin
void DominatorAnalyzer::ComputeDominance() {
    dom.emplace_back(func->GetBlockSize(), false);
    dom[0].Set(0);
    for (auto _ : std::views::iota(1ul, func->GetBlockSize())) {
        dom.emplace_back(func->GetBlockSize(), true);
    }

    auto rpo = GetReversePostOrder(func);

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto *block : rpo) {
            // Only entry block has no predecessor. Entry block's
            // dom set only contains itself
            if (block->GetPredecessorSize()) {
                IndexSet ns(dom[block->GetPredecessor(0)->GetIndex()]);

                for (auto k :
                     std::views::iota(1ul, block->GetPredecessorSize())) {
                    ns = ns & dom[block->GetPredecessor(k)->GetIndex()];
                }
                ns.Set(block->GetIndex());

                if (ns != dom[block->GetIndex()]) {
                    dom[block->GetIndex()] = ns;
                    changed = true;
                }
            }
        }
    }
}

void DominatorAnalyzer::ComputeImmediateDominators() {
    if (dom.size() != func->GetBlockSize()) {
        ComputeDominance();
    }

    idom = std::vector<Block *>(func->GetBlockSize(), nullptr);

    for (auto i : std::views::iota(1ul, func->GetBlockSize())) {
        auto sdom = dom[i];
        sdom.Reset(i);
        auto dominators = sdom.GetBlocks();
        for (auto k : dominators) {
            if (sdom == dom[k]) {
                idom[i] = func->GetBlock(k);
                break;
            }
        }
    }
}

void DominatorAnalyzer::ComputeDominanceFrontier() {
    if (idom.size() != func->GetBlockSize()) {
        ComputeImmediateDominators();
    }

    df = std::vector<IndexSet>(func->GetBlockSize(),
                               IndexSet(func->GetBlockSize()));

    // Since entry block has no predecessors
    for (auto i : std::views::iota(1ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        if (block->GetPredecessorSize() > 1) {
            for (auto k : std::views::iota(0ul, block->GetPredecessorSize())) {
                auto *runner = block->GetPredecessor(k);
                while (runner != idom[i]) {
                    df[runner->GetIndex()].Set(i);
                    runner = idom[runner->GetIndex()];
                }
            }
        }
    }
}

void DominatorAnalyzer::BuildDominatorTree() {
    if (idom.size() != func->GetBlockSize()) {
        ComputeImmediateDominators();
    }

    dtree = std::vector<std::vector<Block *>>(func->GetBlockSize());
    for (auto i : std::views::iota(1ul, idom.size())) {
        dtree[idom[i]->GetIndex()].push_back(func->GetBlock(i));
    }
}

std::vector<Block *>
DominatorAnalyzer::GetDominanceFrontier(Block *block) const {
    auto blocks = df.at(block->GetIndex()).GetBlocks();
    std::vector<Block *> result;
    for (auto i : blocks) {
        result.push_back(func->GetBlock(i));
    }
    return result;
}

void DominatorAnalyzer::DumpDominators(std::ostream &out) const {
    std::cout << "Dominators: " << func->GetName() << "\n";
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        out << block->GetName() << ":";
        for (auto k :
             std::views::iota(0ul, dom.at(block->GetIndex()).GetSize())) {
            if (dom.at(block->GetIndex()).Get(k)) {
                out << "  " << func->GetBlock(k)->GetName();
            }
        }
        out << "\n";
    }
}

void DominatorAnalyzer::DumpImmediateDominators(std::ostream &out) const {
    std::cout << "Immediate Dominators: " << func->GetName() << "\n";
    auto *block = func->GetBlock(0);
    out << block->GetName() << ":";
    out << "  \n";
    for (auto i : std::views::iota(1ul, func->GetBlockSize())) {
        block = func->GetBlock(i);
        out << block->GetName() << ":";
        out << "  " << idom[i]->GetName() << "\n";
    }
}

void DominatorAnalyzer::DumpDominanceFrontier(std::ostream &out) const {
    std::cout << "DominanceFrontier: " << func->GetName() << "\n";
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        auto *block = func->GetBlock(i);
        out << "  " << block->GetName() << ":";
        for (auto k :
             std::views::iota(0ul, df.at(block->GetIndex()).GetSize())) {
            if (df.at(block->GetIndex()).Get(k)) {
                out << "  " << func->GetBlock(k)->GetName();
            }
        }
        out << "\n";
    }
}
// DominatorAnalyzer end

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
