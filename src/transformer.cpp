#include "transformer.hpp"
#include "block.hpp"
#include "function.hpp"
#include "opcodes.hpp"
#include <algorithm>
#include <ranges>
#include <unordered_set>

namespace sc {

// EarlyIRTransformer Begin
void EarlyIRTransformer::FixIR(Function *func) {
    std::vector<Block *> rb;
    for (size_t i : std::views::iota(0UL, func->GetBlockSize()) |
                        std::views::take(func->GetBlockSize() - 1)) {
        auto *block = func->GetBlock(i);
        if (block->GetInstructionSize()) {
            auto *instr = LAST_INSTR(block);
            if (instr->GetOpCode() == OpCode::RET) {
                rb.push_back(block);
            } else if (instr->GetOpCode() != OpCode::BR &&
                       instr->GetOpCode() != OpCode::JMP) {
                auto jmp_instr = std::make_unique<JmpInstruction>();
                auto *nxt_lbl = func->GetBlock(i + 1)->GetLabel();
                jmp_instr->SetOperand(nxt_lbl);
                func->GetBlock(i)->AddInstruction(std::move(jmp_instr));
            }
        }
    }

    auto *block = LAST_BLK(func);
    if (block->GetInstructionSize() &&
        LAST_INSTR(block)->GetOpCode() == OpCode::RET) {
        rb.push_back(block);
    }

    if (rb.size() == 0) {
        assert(func->GetRetType() == DataType::VOID &&
               "Non-void function with no return instruction.\n");
        block->AddInstruction(std::make_unique<RetInstruction>());
        rb.push_back(block);
    }

    // last block can only end in these instructions
    assert(LAST_INSTR(block)->GetOpCode() == OpCode::RET ||
           LAST_INSTR(block)->GetOpCode() == OpCode::JMP ||
           LAST_INSTR(block)->GetOpCode() == OpCode::BR);

    if (rb.size() > 1) {
        AddUniqueExitBlock(std::move(rb), func);
    }
}

void EarlyIRTransformer::AddUniqueExitBlock(std::vector<Block *> rb,
                                            Function *func) {
    // add new exit block
    func->AddBlock(std::make_unique<Block>("exit"));
    // create new lbl operand to store jmp dest
    auto lbl_op = std::make_unique<LabelOperand>("exit");
    lbl_op->SetBlock(LAST_BLK(func));
    LAST_BLK(func)->SetLabel(lbl_op.get());

    // create new ret instruction
    auto ret_instr = std::make_unique<RetInstruction>();
    // create new temp ret operand
    std::unique_ptr<OperandBase> ret_op = nullptr;
    // only add operand if not void
    if (func->GetRetType() != DataType::VOID) {
        if (func->GetRetType() == DataType::PTR) {
            ret_op = std::make_unique<PtrOperand>("__$rret__");
            for (auto dt : static_cast<PtrFunction *>(func)->GetPtrChain()) {
                static_cast<PtrOperand *>(ret_op.get())->AppendPtrChain(dt);
            }
        } else {
            ret_op =
                std::make_unique<RegOperand>(func->GetRetType(), "__$rret__");
        }
        ret_instr->SetOperand(ret_op.get());
    }
    // add new unique ret instr to the exit block
    LAST_BLK(func)->AddInstruction(std::move(ret_instr));

    // modify the return block
    // delete ret instr and change with id and jmp instr
    for (auto *block : rb) {
        auto ret_instr = LAST_INSTR(block);

        auto jmp_instr = std::make_unique<JmpInstruction>();
        jmp_instr->SetOperand(lbl_op.get());

        if (func->GetRetType() != DataType::VOID) {
            auto id_instr = std::make_unique<IdInstruction>();
            id_instr->SetOperand(ret_op.get());
            id_instr->SetOperand(ret_instr->GetOperand(0));
            block->AddInstruction(std::move(id_instr),
                                  block->GetInstructionSize() - 1);
            block->AddInstruction(std::move(jmp_instr));
        } else {
            block->AddInstruction(std::move(jmp_instr),
                                  block->GetInstructionSize() - 1);
        }
    }

    // add ret operand to function
    func->AddOperand(std::move(lbl_op));
    func->AddOperand(std::move(ret_op));
}
// EarlyIRTransformer End

/*
// CFTransformer Begin
void CFTransformer::RemoveUnreachableCFNode(Function *func) {
    std::unordered_set<Block *> reachable(traverse_order.cbegin(),
                                          traverse_order.cend());
    auto &blocks = func->GetBlocks();
    // TODO: This needs fixing
    // deleting blocks invalidates the every other block's index
    // also, invalidates the label operands values
    // instead of storing block index store block pointer
    // get rid of label instr
    blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                                [&reachable](auto &block) {
                                    return !reachable.contains(block.get());
                                }),
                 blocks.end());
}

bool CFTransformer::Clean(Function *func) {
    (void)func;
    bool ret = false;
    for (auto *block : traverse_order) {
        auto instr = LAST_INSTR(block);
        if (instr->GetOpCode() == OpCode::BR &&
            instr->GetOperand(0) == instr->GetOperand(1)) {
            ReplaceBrWithJmp(block);
            ret = true;
        }
        if (instr->GetOpCode() == OpCode::JMP) {
            if (block->GetInstructionSize() == 2) {
                // Empty block one label and one jmp
                ret = true;
            }
            auto *succ_blk = block->GetSuccessor(0);
            if (succ_blk->GetPredecessorSize() == 1) {
                // combine current and next block
                ret = true;
            }
            if (succ_blk->GetInstructionSize() == 2 &&
                LAST_INSTR(succ_blk)->GetOpCode() == OpCode::BR) {
                // change block jmp with succ_blk br
                ret = true;
            }
        }
    }
    return ret;
}

void CFTransformer::ReplaceBrWithJmp(Block *block) {
    auto jmp_instr = std::make_unique<JmpInstruction>();
    jmp_instr->SetOperand(LAST_INSTR(block)->GetOperand(0));
    block->AddInstruction(std::move(jmp_instr),
                          block->GetInstructionSize() - 1);
    block->RemoveSuccessor(block->GetSuccessorSize() - 1);
    auto *succ_blk = block->GetSuccessor(0);

    // The predecessor block should ideally have two predecessor link to current
    // block. Remove the other reference.
    size_t count = 0;
    for (size_t i : std::views::iota(0UL, succ_blk->GetPredecessorSize())) {
        if (succ_blk->GetPredecessor(i) == block) {
            ++count;
            if (count > 1) {
                succ_blk->RemovePredecessor(i);
            }
        }
    }
}
// CFTransformer End
*/

} // namespace sc
