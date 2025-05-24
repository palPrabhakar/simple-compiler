#include "transformers/cf_transformer.hpp"
#include "analyzers/cfg.hpp"
#include <unordered_set>

// #define PRINT_DEBUG
#undef PRINT_DEBUG

namespace sc {
// CFTransformer begin
void CFTransformer::Transform() {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Processing Function:  " << func->GetName() << "\n";
#endif
    do {
        traverse_order = GetPostOrder(func);
    } while (Clean());

    // Remove all unreachable nodes
    traverse_order = GetPostOrder(func);
    if (traverse_order.size() != func->GetBlockSize()) {
        RemoveUnreachableCFNode();
    }
#ifdef PRINT_DEBUG
    std::cout<<"\n";
#endif
}

bool CFTransformer::Clean() {
    bool ret = false;
    for (auto *block : traverse_order) {
#ifdef PRINT_DEBUG
        std::cout << __PRETTY_FUNCTION__ << " Clean:  " << block->GetName()
                  << "\n";
#endif
        auto instr = LAST_INSTR(block);
        if (instr->GetOpcode() == Opcode::BR &&
            static_cast<BranchInstruction *>(instr)->GetTrueDest() ==
                static_cast<BranchInstruction *>(instr)->GetFalseDest()) {
            ReplaceBrWithJmp(block);
            ret = true;
        }
        if (instr->GetOpcode() == Opcode::JMP) {
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
                LAST_INSTR(succ_blk)->GetOpcode() == Opcode::BR) {
                // change block jmp with succ_blk br
                HoistBranch(block);
                ret = true;
            }
        }
    }

#ifdef PRINT_DEBUG
    std::cout << "\n";
#endif

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
    for (auto *pred : succ_blk->GetPredecessors()) {
        if (pred == block) {
            ++count;
            if (count > 1) {
                succ_blk->RemovePredecessor(pred);
            }
        }
    }

    assert(count > 1);
}

void CFTransformer::RemoveEmptyBlock(Block *block) {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__ << " Removing block: " << block->GetName()
              << "\n";
#endif
    auto *succ_blk = block->GetSuccessor(0);
    succ_blk->RemovePredecessor(block);

    // remove the successor since clean is
    // implemented as a series of if's
    block->RemoveSuccessor(succ_blk);

    for (auto *pred_blk : block->GetPredecessors()) {
        auto lst_instr = LAST_INSTR(pred_blk);
        if (lst_instr->GetOpcode() == Opcode::JMP) {
            static_cast<JmpInstruction *>(lst_instr)->SetJmpDest(
                succ_blk->GetLabel());
            pred_blk->AddSuccessor(succ_blk, 0);
        } else {
            assert(lst_instr->GetOpcode() == Opcode::BR);
            auto *br = static_cast<BranchInstruction *>(lst_instr);
            if (br->GetTrueDest()->GetBlock() == block) {
                br->SetTrueDest(succ_blk->GetLabel());
                assert(pred_blk->GetSuccessor(0) == block);
                pred_blk->AddSuccessor(succ_blk, 0);
            }

            if (br->GetFalseDest()->GetBlock() == block) {
                br->SetFalseDest(succ_blk->GetLabel());
                assert(pred_blk->GetSuccessor(1) == block);
                pred_blk->AddSuccessor(succ_blk, 1);
            }
        }
        succ_blk->AddPredecessor(pred_blk);
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

    block->RemoveSuccessor(static_cast<size_t>(0));

    for (auto *ssucc : succ_blk->GetSuccessors()) {
        ssucc->RemovePredecessor(succ_blk);
        ssucc->AddPredecessor(block);
        block->AddSuccessor(ssucc);
    }
}

void CFTransformer::HoistBranch(Block *block) {
    auto *succ_blk = block->GetSuccessor(0);
    succ_blk->RemovePredecessor(block);
    block->RemoveSuccessor(succ_blk);

#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Hoisting branch from: " << succ_blk->GetName()
              << " to: " << block->GetName() << "\n";
#endif

    auto *last_instr = static_cast<BranchInstruction *>(LAST_INSTR(succ_blk));
    auto br_instr = std::make_unique<BranchInstruction>();

    // set the true lbl
    auto *true_dest = last_instr->GetTrueDest();
    br_instr->SetTrueDest(true_dest);
    true_dest->GetBlock()->AddPredecessor(block);
    block->AddSuccessor(true_dest->GetBlock());

    // set the false lbl
    auto *false_lbl = last_instr->GetFalseDest();
    br_instr->SetFalseDest(false_lbl);
    false_lbl->GetBlock()->AddPredecessor(block);
    block->AddSuccessor(false_lbl->GetBlock());

    br_instr->SetOperand(last_instr->GetOperand(0));

    block->AddInstruction(std::move(br_instr), block->GetInstructionSize() - 1);
}

void CFTransformer::RemoveUnreachableCFNode() {
    std::unordered_set<Block *> reachable(traverse_order.cbegin(),
                                          traverse_order.cend());
    std::vector<size_t> remove_blks;
    for (auto *blk : func->GetBlocks()) {
        if (!reachable.contains(blk)) {
            remove_blks.push_back(blk->GetIndex());
        }
    }
    func->RemoveBlocks(std::move(remove_blks));
}
// CFTransformer end
} // namespace sc
