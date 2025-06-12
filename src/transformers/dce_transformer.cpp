#include "transformers/dce_transformer.hpp"
#include "block.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"
#include "operand.hpp"
#include <memory>

namespace sc {
void DCETransformer::Transform() {
    dom.ComputeDominanceFrontier();
    Mark();
    Sweep();
}

void DCETransformer::Mark() {
    std::vector<InstructionBase *> worklist;

    for (auto *block : func->GetBlocks()) {
        auto idx = 0ul;
        imarks[block->GetIndex()] =
            std::vector<bool>(block->GetInstructionSize(), false);

        for (auto *instr : block->GetInstructions()) {
            assert(instr->GetIndex() == idx++);
            if (Critical(instr)) {
                Mark(instr);
                worklist.push_back(instr);
            }
        }
    }

    for (size_t i = 0; i < worklist.size(); ++i) {
        auto *instr = worklist[i];

        if (instr->GetOpcode() == Opcode::GET) {
            auto *geti = static_cast<GetInstruction *>(instr);
            for (auto *seti : geti->GetSetPairs()) {
                if (!CheckAndMark(seti)) {
                    worklist.push_back(seti);
                }
            }
        } else {
            for (auto *operands : instr->GetOperands()) {
                auto *def = operands->GetDef();
                if (def && !CheckAndMark(def)) {
                    worklist.push_back(def);
                }
            }
        }

        for (auto *blk : dom.GetDominanceFrontier(instr->GetBlock())) {
            auto *last_instr = LAST_INSTR(blk);
            if (last_instr->GetOpcode() == Opcode::BR &&
                !CheckAndMark(last_instr)) {
                worklist.push_back(last_instr);
            }
        }
    }
}

void DCETransformer::Sweep() {
    std::vector<size_t> remove_list;
    for (auto bi : std::views::iota(0ul, func->GetBlockSize())) {
        remove_list.clear();
        auto *block = func->GetBlock(bi);

        for (auto i : std::views::iota(0ul, imarks[bi].size())) {
            if (!imarks[bi][i]) {
                auto *instr = block->GetInstruction(i);

                if (instr->GetOpcode() == Opcode::BR) {
                    auto curr = block->GetIndex();
                    do {
                        // post-dominator
                        auto *pdom = dom.GetImmediateDominator(curr);
                        assert(pdom);
                        if (bmarks[pdom->GetIndex()]) {
                            auto jmp_inst = std::make_unique<JmpInstruction>();
                            jmp_inst->SetBlock(block);
                            jmp_inst->SetJmpDest(pdom->GetLabel());

                            block->AddInstruction(
                                std::move(jmp_inst),
                                block->GetInstructionSize() - 1, true);
                        } else {
                            curr = pdom->GetIndex();
                        }
                    } while (true);
                } else if (instr->GetOpcode() != Opcode::JMP) {
                    remove_list.push_back(i);
                }
            }
        }

        block->RemoveInstructions(remove_list, true);
    }
}

bool DCETransformer::Critical(InstructionBase *instr) {
    switch (instr->GetOpcode()) {
    case Opcode::RET:
    case Opcode::PRINT:
    case Opcode::CALL:
        /*
         * Since Call's can have side-effects
         * The calling function might effect a
         * global storage, or it might print.
         */
    case Opcode::ALLOC:
    case Opcode::FREE:
    case Opcode::STORE:
    case Opcode::GETARG:
        /*
         * It might be possible that the storage
         * location created using alloc is never
         * written (this will require proving the
         * pointer value never leaves the function!).
         * Also the complexity of tracking free or
         * memory-leaks make this problem extremely
         * challenging (don't want to mark the free
         * statement dead when it is not!)
         */
        return true;
    default:
        return false;
    }
}

bool DCETransformer::CheckAndMark(InstructionBase *instr) {
    auto mark = Check(instr);
    if (!mark) {
        Mark(instr);
    }
    return mark;
}
} // namespace sc
