#include "transformers/cf_transformer.hpp"
#include "analyzers/cfg.hpp"
#include <unordered_set>

#undef PRINT_DEBUG

namespace sc {
static void RemovePredecessorIf(Block *block, Block *match) {
    for (size_t i : std::views::iota(0UL, block->GetPredecessorSize())) {
        if (block->GetPredecessor(i) == match) {
            block->RemovePredecessor(i);
            return;
        }
    }
}

static void RemoveSuccessorIf(Block *block, Block *match) {
    for (size_t i : std::views::iota(0UL, block->GetSuccessorSize())) {
        if (block->GetSuccessor(i) == match) {
            block->RemoveSuccessor(i);
            return;
        }
    }
}

// CFTransformer begin
void CFTransformer::Transform() {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Processing Function:  " << f->GetName() << "\n";
#endif
    do {
        traverse_order = GetPostOrder(func);
    } while (Clean());

    // Remove all unreachable nodes
    traverse_order = GetPostOrder(func);
    if (traverse_order.size() != func->GetBlockSize()) {
        RemoveUnreachableCFNode();
    }
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
            // Do not remove the block if it's the entry block
            // unique entry block! Otherwise there won't be an
            // unique exit block in reverse CFG
            if (block->GetInstructionSize() == 1 &&
                block->GetPredecessorSize()) {
                // Empty block one label and one jmp
                RemoveEmptyBlock(block);
                ret = true;
            }
            // exit node has no successor
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
        RemovePredecessorIf(succ_blk->GetSuccessor(i), succ_blk);
        succ_blk->GetSuccessor(i)->AddPredecessor(block);
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

void CFTransformer::RemoveUnreachableCFNode() {
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

    // fix block indexes
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        func->GetBlock(i)->SetIndex(i);
    }
}
// CFTransformer end
} // namespace sc
