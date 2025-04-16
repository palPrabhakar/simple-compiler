#include "transformer.hpp"
#include <ranges>

namespace sc {
void EarlyIRTransformer::FixIR(Function *func) {
    std::vector<Block *> rb;
    for (size_t i : std::views::iota(0UL, func->GetBlockSize())) {
        auto *instr = func->GetBlock(i)->GetInstruction(0);
        func->GetBlock(i)->SetId(i);
        assert(instr->GetOpCode() == OpCode::LABEL &&
               "LabelInstruction expected Function::FixLabels\n");
        static_cast<LabelOperand *>(instr->GetOperand(0))->SetBlockIdx(i);
        instr = LAST_INSTR(func->GetBlock(i));
        if (instr->GetOpCode() == OpCode::RET) {
            rb.push_back(func->GetBlock(i));
        }
    }

    if (rb.size() > 1) {
        AddUniqueExitBlock(std::move(rb), func);
    }

    // auto &block = blocks[blocks.size() - 1];
    if (LAST_INSTR(LAST_BLK(func))->GetOpCode() != OpCode::RET) {
        assert(func->GetRetType() == DataType::VOID &&
               "Non-void with non-return last instruction.\n");
        LAST_BLK(func)->AddInstruction(std::make_unique<RetInstruction>());
    }
}

void EarlyIRTransformer::AddUniqueExitBlock(std::vector<Block *> rb,
                                            Function *func) {
    // add new exit block
    func->AddBlock(std::make_unique<Block>("exit"));
    LAST_BLK(func)->SetId(func->GetBlockSize() - 1);

    // create new lbl instr and op
    auto lbl_instr = std::make_unique<LabelInstruction>();
    auto lbl_op = std::make_unique<LabelOperand>("exit");
    lbl_op->SetBlockIdx(LAST_BLK(func)->GetId());
    lbl_instr->SetOperand(lbl_op.get());
    // add lbl instruction to new block
    LAST_BLK(func)->AddInstruction(std::move(lbl_instr));

    // create new ret instr and new tmp ret op
    auto ret_instr = std::make_unique<RetInstruction>();
    std::unique_ptr<OperandBase> ret_op = nullptr;
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
} // namespace sc
