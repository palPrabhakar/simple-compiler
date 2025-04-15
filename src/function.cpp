#include "function.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"
#include "operand.hpp"
#include <ranges>

namespace sc {
void Function::Dump(std::ostream &out) {
    out << "@" << name;
    if (args.size()) {
        out << "(";
        out << args[0]->GetName() << ": " << args[0]->GetStrType();
        for (size_t i = 1; i < args.size(); ++i) {
            out << ", " << args[i]->GetName() << ": " << args[i]->GetStrType();
        }
        out << ")";
    }
    if (ret_type != DataType::VOID) {
        out << ": " << GetStrRetType();
    }
    out << " {\n";
    for (auto &block : blocks) {
        for (auto &instr : *block) {
            if (instr->GetOpCode() == OpCode::LABEL) {
                instr->Dump(out);
            } else {
                instr->Dump(out << "  ");
            }
        }
    }
    out << "}\n";
}

std::string Function::GetStrRetType() const {
    return ret_type != DataType::VOID ? GetStrDataType(ret_type)
                                      : std::string();
}

void Function::FixIR() {
    std::vector<Block *> rb;
    for (size_t i : std::views::iota(0UL, blocks.size())) {
        auto *instr = blocks[i]->GetInstruction(0);
        assert(instr->GetOpCode() == OpCode::LABEL &&
               "LabelInstruction expected Function::FixLabels\n");
        static_cast<LabelOperand *>(instr->GetOperand(0))->SetBlockIdx(i);

        instr = blocks[i]->GetInstruction(blocks[i]->GetInstructionSize() - 1);
        if (instr->GetOpCode() == OpCode::RET) {
            rb.push_back(blocks[i].get());
        }
    }

    if (rb.size() > 1) {
        AddUniqueExitBlock(std::move(rb));
    }

    auto &block = blocks[blocks.size() - 1];
    if (block->GetInstruction(block->GetInstructionSize() - 1)->GetOpCode() !=
        OpCode::RET) {
        assert(ret_type == DataType::VOID &&
               "Non-void with non-return last instruction.\n");
        block->AddInstruction(std::make_unique<RetInstruction>());
    }
}

void Function::AddUniqueExitBlock(std::vector<Block *> rb) {
    // add new exit block
    blocks.push_back(std::make_unique<Block>("exit"));

    // create new lbl instr and op
    auto lbl_instr = std::make_unique<LabelInstruction>();
    auto lbl_op = std::make_unique<LabelOperand>("exit");
    lbl_op->SetBlockIdx(blocks.size() - 1);
    lbl_instr->SetOperand(lbl_op.get());
    // add lbl instruction to new block
    blocks[blocks.size() - 1]->AddInstruction(std::move(lbl_instr));

    // create new ret instr and new tmp ret op
    auto ret_instr = std::make_unique<RetInstruction>();
    std::unique_ptr<OperandBase> ret_op = nullptr;
    if (ret_type != DataType::VOID) {
        if (ret_type == DataType::PTR) {
            ret_op = std::make_unique<PtrOperand>("__$rret__");
            for (auto dt : static_cast<PtrFunction *>(this)->GetPtrChain()) {
                static_cast<PtrOperand *>(ret_op.get())->AppendPtrChain(dt);
            }
        } else {
            ret_op = std::make_unique<RegOperand>(ret_type, "__$rret__");
        }
        ret_instr->SetOperand(ret_op.get());
    }
    // add new unique ret instr to the exit block
    blocks[blocks.size() - 1]->AddInstruction(std::move(ret_instr));

    // modify the return block
    // delete ret instr and change with id and jmp instr
    for (auto *block : rb) {
        auto ret_instr = block->GetInstruction(block->GetInstructionSize() - 1);

        auto jmp_instr = std::make_unique<JmpInstruction>();
        jmp_instr->SetOperand(lbl_op.get());

        if (ret_type != DataType::VOID) {
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
    operands.push_back(std::move(lbl_op));
    operands.push_back(std::move(ret_op));
}

void Function::BuildCFG() {
    FixIR();
    auto add_successor = [&blocks = this->blocks](InstructionBase *instr,
                                                  size_t idx, size_t start,
                                                  size_t end) {
        for (size_t i : std::views::iota(start, end)) {
            auto si = static_cast<LabelOperand *>(instr->GetOperand(i))
                          ->GetBlockIdx();
            blocks[idx]->AddSuccessor(blocks[si].get());
        }
    };
    for (size_t i : std::views::iota(0UL, blocks.size())) {
        auto instr =
            blocks[i]->GetInstruction(blocks[i]->GetInstructionSize() - 1);
        switch (instr->GetOpCode()) {
        case OpCode::JMP:
            add_successor(instr, i, 0, 1);
            break;
        case OpCode::BR:
            add_successor(instr, i, 0, 2);
            break;
        case OpCode::RET:
            assert(i == blocks.size() - 1 &&
                   "Function::BuildCFG - ret instruction found befor last "
                   "block\n");
            break;
        default:
            assert(i + 1 < blocks.size() &&
                   "Function::BuildCFG - Last block should end with ret "
                   "instruction \n");
            blocks[i]->AddSuccessor(blocks[i + 1].get());
        }
    }
}
void Function::DumpBlocks(std::ostream &out) {
    out << "function: " << name << "\n";
    for (auto &block : blocks) {
        out << "  " << block->GetName() << "\n";
    }
    out << "\n";
}

void Function::DumpCFG(std::ostream &out) {
    out << "function: " << name << "\n";
    for (auto &block : blocks) {
        out << "  " << block->GetName() << ": ";
        for (auto succ : block->GetSuccessors()) {
            out << succ->GetName() << " ";
        }
        out << "\n";
    }
    out << "\n";
}

std::string PtrFunction::GetStrRetType() const { return GetPtrType(ptr_chain); }
} // namespace sc
