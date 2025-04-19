#include "transformer.hpp"
#include "block.hpp"
#include "function.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"
#include "operand.hpp"
#include <algorithm>
#include <cassert>
#include <ranges>
#include <unordered_set>

namespace sc {

#undef PRINT_DEBUG

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
        } else {
            auto jmp_instr = std::make_unique<JmpInstruction>();
            auto *nxt_lbl = func->GetBlock(i + 1)->GetLabel();
            jmp_instr->SetOperand(nxt_lbl);
            func->GetBlock(i)->AddInstruction(std::move(jmp_instr));
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
            ret_op = std::make_unique<PtrOperand>("__rret__");
            for (auto dt : static_cast<PtrFunction *>(func)->GetPtrChain()) {
                static_cast<PtrOperand *>(ret_op.get())->AppendPtrChain(dt);
            }
        } else {
            ret_op =
                std::make_unique<RegOperand>(func->GetRetType(), "__rret__");
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

// CFTransformer Begin
std::unique_ptr<Program>
CFTransformer::Transform(std::unique_ptr<Program> program) {
    for (auto &f : *program) {
        do {
            traverse_order = GetPostOrder(f.get());
        } while (Clean());

        // Remove all unreachable nodes
        traverse_order = GetPostOrder(f.get());
        if (traverse_order.size() != f->GetBlockSize()) {
            RemoveUnreachableCFNode(f.get());
        }
    }
    return program;
}

bool CFTransformer::Clean() {
    bool ret = false;
    for (auto *block : traverse_order) {
#ifdef PRINT_DEBUG
        std::cout << __PRETTY_FUNCTION__ << " Clean:  " << block->GetName()
                  << "\n";
#endif
        auto instr = LAST_INSTR(block);
        if (instr->GetOpCode() == OpCode::BR &&
            instr->GetOperand(0) == instr->GetOperand(1)) {
            ReplaceBrWithJmp(block);
            ret = true;
        }
        if (instr->GetOpCode() == OpCode::JMP) {
            if (block->GetInstructionSize() == 1) {
                // Empty block one label and one jmp
                RemoveEmptyBlock(block);
                ret = true;
            }
            // root node has no successor
            auto *succ_blk =
                block->GetSuccessorSize() ? block->GetSuccessor(0) : nullptr;
            if (succ_blk && succ_blk->GetPredecessorSize() == 1) {
                // combine current and next block
                CombineBlocks(block);
                ret = true;
            }
            if (succ_blk && succ_blk->GetInstructionSize() == 1 &&
                LAST_INSTR(succ_blk)->GetOpCode() == OpCode::BR) {
                // change block jmp with succ_blk br
                HoistBranch(block);
                ret = true;
            }
        }
    }
    return ret;
}

void CFTransformer::ReplaceBrWithJmp(Block *block) {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Replacing branch with jmp in: " << block->GetName() << "\n";
#endif
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

void CFTransformer::RemoveEmptyBlock(Block *block) {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__ << " Removing block: " << block->GetName()
              << "\n";
#endif
    auto *succ_blk = block->GetSuccessor(0);
    RemovePredecessorIf(succ_blk, block);

    // remove the successor since clean is
    // implemented as a series of if's
    RemoveSuccessorIf(block, succ_blk);

    for (size_t i : std::views::iota(0UL, block->GetPredecessorSize())) {
        auto pred_blk = block->GetPredecessor(i);
        auto lst_instr = LAST_INSTR(pred_blk);
        if (lst_instr->GetOpCode() == OpCode::JMP) {
            lst_instr->SetOperand(succ_blk->GetLabel(), 0);
            pred_blk->AddSuccessor(succ_blk, 0);
        } else {
            assert(lst_instr->GetOpCode() == OpCode::BR);
            if (static_cast<LabelOperand *>(lst_instr->GetOperand(0))
                    ->GetBlock() == block) {
                lst_instr->SetOperand(succ_blk->GetLabel(), 0);
                pred_blk->AddSuccessor(succ_blk, 0);
            }

            if (static_cast<LabelOperand *>(lst_instr->GetOperand(1))
                    ->GetBlock() == block) {
                lst_instr->SetOperand(succ_blk->GetLabel(), 1);
                pred_blk->AddSuccessor(succ_blk, 1);
            }
        }
        succ_blk->AddPredecessor(pred_blk);
    }

    for (size_t i : std::views::iota(0UL, block->GetPredecessorSize()) |
                        std::views::reverse) {
        block->RemovePredecessor(i);
    }
}

void CFTransformer::CombineBlocks(Block *block) {
    // remove the jmp instruction from the block
    block->RemoveInstruction(block->GetInstructionSize() - 1);

    // Add succ blocks instr into current block
    auto *succ_blk = block->GetSuccessor(0);

#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Combining block: " << succ_blk->GetName()
              << " into: " << block->GetName() << "\n";
#endif

    auto instructions = succ_blk->ReleaseInstructions();
    for (size_t i : std::views::iota(0UL, instructions.size())) {
        block->AddInstruction(std::move(instructions[i]));
    }

    block->RemoveSuccessor(0);

    for (size_t i : std::views::iota(0UL, succ_blk->GetSuccessorSize())) {
        block->AddSuccessor(succ_blk->GetSuccessor(i));
    }
}

void CFTransformer::HoistBranch(Block *block) {
    auto *succ_blk = block->GetSuccessor(0);
    RemovePredecessorIf(succ_blk, block);
    RemoveSuccessorIf(block, succ_blk);

#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Hoisting branch from: " << succ_blk->GetName()
              << " to: " << block->GetName() << "\n";
#endif

    auto *last_instr = LAST_INSTR(succ_blk);
    auto br_instr = std::make_unique<BranchInstruction>();
    for (size_t i : std::views::iota(0UL, 2UL)) {
        auto *op = static_cast<LabelOperand *>(last_instr->GetOperand(i));
        br_instr->SetOperand(op);
        block->AddSuccessor(op->GetBlock());
        op->GetBlock()->AddPredecessor(block);
    }
    br_instr->SetOperand(last_instr->GetOperand(2));

    block->AddInstruction(std::move(br_instr), block->GetInstructionSize() - 1);
}

void CFTransformer::RemoveUnreachableCFNode(Function *func) {
    std::unordered_set<Block *> reachable(traverse_order.cbegin(),
                                          traverse_order.cend());
    auto &blocks = func->GetBlocks();
    blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                                [&reachable](auto &block) {
                                    if (!reachable.contains(block.get())) {
#ifdef PRINT_DEBUG
                                        std::cout << __PRETTY_FUNCTION__
                                                  << " Removing block: "
                                                  << block->GetName() << "\n";
#endif
                                        // TODO:
                                        // erase the corresponding LabelOperand
                                        return true;
                                    }
                                    return false;
                                }),
                 blocks.end());
}
// CFTransformer End

} // namespace sc
