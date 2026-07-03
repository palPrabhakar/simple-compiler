#include "analyzers/live_range.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"
#include "operand.hpp"

namespace sc {
void SSALiveRanges::ComputeLiveRanges() {
    for (auto *blk : func->GetBlocks()) {
        for (auto *instr : blk->GetInstructions()) {
            if (instr->HasDest()) {
                if (instr->GetOpcode() == Opcode::GET) {
                    //  merge live ranges
                    auto *geti = static_cast<GetInstruction *>(instr);
                    std::vector<OperandBase *> oprnds;
                    for (auto *seti : geti->GetSetPairs()) {
                        oprnds.push_back(seti->GetOperand(0));
                    }
                    MergeRanges(geti->GetDest(), oprnds);
                } else {
                    CreateRange(instr->GetDest());
                }
            }
        }
    }
}

void SSALiveRanges::CreateRange(OperandBase *oprnd) {
    ds.MakeSet(oprnd->GetName());
}

void SSALiveRanges::MergeRanges(OperandBase *op,
                                std::vector<OperandBase *> oprnds) {
    auto _set = ds.FindSet(op->GetName());
    for (auto *oprnd : oprnds) {
        ds.UnionSet(_set, oprnd->GetName());
    }
}

bool SSALiveRanges::SameRange(OperandBase *op0, OperandBase *op1) {
    return ds.FindSet(op0->GetName()) == ds.FindSet(op1->GetName());
}

} // namespace sc
