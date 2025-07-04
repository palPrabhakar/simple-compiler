#include "transformers/early_ir_transformer.hpp"
#include "instruction.hpp"
#include <memory>

// #define PRINT_DEBUG
#undef PRINT_DEBUG

namespace sc {
// EarlyIRTransformer begin
void EarlyIRTransformer::Transform() {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << "Processing function:  " << func->GetName() << "\n";
#endif


    CanonicalizeIR();
}

void EarlyIRTransformer::CanonicalizeIR() {
    for (auto *block :
         func->GetBlocks() | std::views::take(func->GetBlockSize() - 1)) {
#ifdef PRINT_DEBUG
        std::cout << __PRETTY_FUNCTION__
                  << " Processing block:  " << block->GetName() << "\n";
#endif
        if (block->GetInstructionSize()) {
            auto *instr = LAST_INSTR(block);
            if (instr->GetOpcode() == Opcode::RET) {
                rb.push_back(block);
            } else if (instr->GetOpcode() != Opcode::BR &&
                       instr->GetOpcode() != Opcode::JMP) {
                InsertJmpInstruction(block,
                                     func->GetBlock(block->GetIndex() + 1));
            }
        } else {
            InsertJmpInstruction(block, func->GetBlock(block->GetIndex() + 1));
        }
    }

    auto *block = LAST_BLK(func);
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Processing block:  " << block->GetName() << "\n";
#endif
    if (block->GetInstructionSize() &&
        LAST_INSTR(block)->GetOpcode() == Opcode::RET) {
        rb.push_back(block);
    } else if (!block->GetInstructionSize() || !rb.size()) {
        // can only add a ret instr if function is void
        assert(func->GetRetType() == DataType::VOID &&
               "Non-void function with no return instruction.\n");
        auto ret_instr = std::make_unique<RetInstruction>();
        ret_instr->SetOperand(VoidOperand::GetVoidOperand().get());
        block->AddInstruction(std::move(ret_instr));
        rb.push_back(block);
    }

    // last block can only end in these instructions
    assert(LAST_INSTR(block)->GetOpcode() == Opcode::RET ||
           LAST_INSTR(block)->GetOpcode() == Opcode::JMP ||
           LAST_INSTR(block)->GetOpcode() == Opcode::BR);

    if (rb.size() > 1) {
        AddUniqueExitBlock();
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

    // create new lbl operand to store jmp dest
    auto lbl_op = std::make_unique<LabelOperand>("__sc_exit__");
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
            ret_op = std::make_shared<PtrOperand>("__rret__");
            for (auto dt : static_cast<PtrFunction *>(func)->GetPtrChain()) {
                static_cast<PtrOperand *>(ret_op.get())->AppendPtrChain(dt);
            }
        } else {
            ret_op =
                std::make_shared<RegOperand>(func->GetRetType(), "__rret__");
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

void EarlyIRTransformer::InsertJmpInstruction(Block *blk, Block *jmp_blk) {
    auto jmp_instr = std::make_unique<JmpInstruction>();
    jmp_instr->SetJmpDest(jmp_blk->GetLabel());
    blk->AddInstruction(std::move(jmp_instr));
}
// EarlyIRTransformer end
} // namespace sc
