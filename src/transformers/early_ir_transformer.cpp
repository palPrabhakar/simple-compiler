#include "transformers/early_ir_transformer.hpp"
#include "block.hpp"
#include "function.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"
#include <memory>

// #define PRINT_DEBUG
#undef PRINT_DEBUG

namespace sc {

bool check_dest(InstructionBase *instr, std::string dest_name) {
    // Helper function to check if the destination of jmp
    // or br instruction is same as dest_name
    if (instr->GetOpcode() == Opcode::BR) {
        // br instr
        auto *br = static_cast<BranchInstruction *>(instr);
        if (br->GetTrueDest()->GetName() == dest_name ||
            br->GetFalseDest()->GetName() == dest_name) {
            return true;
        }
    } else {
        // jmp instr
        auto *jmp = static_cast<JmpInstruction *>(instr);
        if (jmp->GetJmpDest()->GetName() == dest_name) {
            return true;
        }
    }
    return false;
}

bool is_jmp_or_br(InstructionBase *instr) {
    return instr->GetOpcode() == Opcode::JMP ||
           instr->GetOpcode() == Opcode::BR;
}

// EarlyIRTransformer begin
void EarlyIRTransformer::Transform() {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << "Processing function:  " << func->GetName() << "\n";
#endif

    CanonicalizeIR();
}

void EarlyIRTransformer::CanonicalizeIR() {
    bool add_entry_blk = false;
    for (auto *block :
         func->GetBlocks() | std::views::take(func->GetBlockSize() - 1)) {

#ifdef PRINT_DEBUG
        std::cout << __PRETTY_FUNCTION__
                  << " Processing block:  " << block->GetName() << "\n";
#endif

        if (block->GetInstructionSize()) {
            auto *instr = LAST_INSTR(block);
            if (instr->GetOpcode() == Opcode::RET) {
                // block contains a return instr record
                // to check if unique exit block required
                rb.push_back(block);
            } else if (!is_jmp_or_br(instr)) {
                // the block doesn't end with a br or jmp instr
                // add a jmp instr to the next block
                InsertJmpInstruction(block,
                                     func->GetBlock(block->GetIndex() + 1));
            } else {
                // if check required unique entry block required
                add_entry_blk = check_dest(instr, func->GetBlock(0)->GetName());
            }
        } else {
            // empty block: insert a jmp instr to the next block
            InsertJmpInstruction(block, func->GetBlock(block->GetIndex() + 1));
        }
    }

    /*
     * Process the last block
     */
    auto *block = LAST_BLK(func);

#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Processing block:  " << block->GetName() << "\n";
#endif

    if (block->GetInstructionSize()) {
        // last block contains instructions
        auto *instr = LAST_INSTR(block);
        if (instr->GetOpcode() == Opcode::RET) {
            rb.push_back(block);
        } else if (is_jmp_or_br(instr)) {
            // The last instructin of the last block is not ret.
            // If there are no other ret statements in the code
            // then the code cannot terminate! At least one ret
            // statement is required
            assert(rb.size() && "error: no ret statement found!");
            add_entry_blk = check_dest(instr, func->GetBlock(0)->GetName());
        } else {
            // add ret instruction
            InsertRetInstruction(block);
        }
    } else {
        // last block contains no instruction add ret instr
        InsertRetInstruction(block);
    }

    // last block can only end in these instructions
    assert(LAST_INSTR(block)->GetOpcode() == Opcode::RET ||
           LAST_INSTR(block)->GetOpcode() == Opcode::JMP ||
           LAST_INSTR(block)->GetOpcode() == Opcode::BR);

    if(add_entry_blk) {
        AddUniqueEntryBlock();
    }

    if (rb.size() > 1) {
        AddUniqueExitBlock();
    } else {
        assert(rb.size() == 1 && "error: expected unique exit block\n");
        func->SetExitBlock(rb[0]);
    }
}

void EarlyIRTransformer::AddUniqueExitBlock() {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << "Processing function:  " << func->GetName() << "\n";
#endif
    // add new exit block
    func->AddBlock(std::make_unique<Block>("__sc_exit__"));
    LAST_BLK(func)->SetIndex(func->GetBlockSize() - 1);
    func->SetExitBlock(LAST_BLK(func));

    // create new lbl operand to store jmp dest
    auto lbl_op = std::make_unique<LabelOperand>("__sc_exit__", func->ctx.get_next_id());
    lbl_op->SetBlock(LAST_BLK(func));
    auto *exit_lbl = lbl_op.get();
    LAST_BLK(func)->SetLabel(std::move(lbl_op));

    // create new ret instruction
    auto ret_instr = std::make_unique<RetInstruction>();
    // create new temp ret operand
    std::shared_ptr<OperandBase> ret_op = nullptr;
    // only add operand if not void
    if (func->GetRetType() != DataType::VOID) {
        if (func->GetRetType() == DataType::PTR) {
            ret_op = std::make_shared<PtrOperand>("__rret__", func->ctx.get_next_id());
            for (auto dt : static_cast<PtrFunction *>(func)->GetPtrChain()) {
                static_cast<PtrOperand *>(ret_op.get())->AppendPtrChain(dt);
            }
        } else {
            ret_op =
                std::make_shared<RegOperand>(func->GetRetType(), "__rret__", func->ctx.get_next_id());
        }
        ret_instr->SetOperand(ret_op.get());
    } else {
        ret_instr->SetOperand(VoidOperand::GetVoidOperand().get());
    }
    // add new unique ret instr to the exit block
    LAST_BLK(func)->AddInstruction(std::move(ret_instr));

    // modify the return block
    // delete ret instr and change with id and jmp instr
    for (auto *block : rb) {
        auto *ret_instr = LAST_INSTR(block);
        auto jmp_instr = std::make_unique<JmpInstruction>();
        jmp_instr->SetJmpDest(exit_lbl);

        if (func->GetRetType() != DataType::VOID) {
            auto id_instr = std::make_unique<IdInstruction>();
            id_instr->SetOperand(ret_instr->GetOperand(0));

            // Still not in SSA-form need to store the reference
            // to the same ret_op in multiple isntructions
            id_instr->AddDest(ret_op);

            block->AddInstruction(std::move(id_instr),
                                  block->GetInstructionSize() - 1);
            block->AddInstruction(std::move(jmp_instr));
        } else {
            // last instruction is ret instruction
            block->AddInstruction(std::move(jmp_instr),
                                  block->GetInstructionSize() - 1);
        }
    }
}

void EarlyIRTransformer::AddUniqueEntryBlock() {
    auto blk = std::make_unique<Block>("__sc_entry__");

    // If function has args copy them from the current
    // entry_block to this new entry_block
    if(func->HasArgs()) {
        // current entry block
        auto eblk = func->GetEntryBlock();
        auto instrs = eblk->ReleaseInstructions(0, func->GetArgsSize() - 1);
        blk->InsertInstructions(std::move(instrs), 0);
    }

    InsertJmpInstruction(blk.get(), func->GetBlock(0));
    func->SetEntryBlock(blk.get());
    func->InsertBlock(std::move(blk), 0);
}

void EarlyIRTransformer::InsertJmpInstruction(Block *blk, Block *jmp_blk) {
    auto jmp_instr = std::make_unique<JmpInstruction>();
    jmp_instr->SetJmpDest(jmp_blk->GetLabel());
    blk->AddInstruction(std::move(jmp_instr));
}

void EarlyIRTransformer::InsertRetInstruction(Block *blk) {
    assert(func->GetRetType() == DataType::VOID &&
           "Non-void function with no return instruction.\n");

    auto ret_instr = std::make_unique<RetInstruction>();
    ret_instr->SetOperand(VoidOperand::GetVoidOperand().get());
    blk->AddInstruction(std::move(ret_instr));
    rb.push_back(blk);
}
// EarlyIRTransformer end
} // namespace sc
