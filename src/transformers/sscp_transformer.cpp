#include "transformers/sscp_transformer.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"

namespace sc {

// #define PRINT_DEBUG
#undef PRINT_DEBUG

void RemoveSetInstruction(GetInstruction *instr) {
    for (auto *seti : instr->GetSetPairs()) {
        auto *sblk = seti->GetBlock();
        sblk->RemoveInstruction(seti->GetIndex(), true);
    }
}

void SSCPTransformer::Initialize() {
    for (auto *blk : func->GetBlocks()) {
        for (auto *inst : blk->GetInstructions()) {
            if (inst->HasDest()) {
                switch (inst->GetOpcode()) {
                case Opcode::GET:
                    values[inst->GetDest()] = LVT::UNKNOWN;
                    break;
                case Opcode::GETARG:
                    /*
                     * Without inter-procedure analysis
                     * function arguments are like reading
                     * form external source.
                     */
                case Opcode::ALLOC:
                    /*
                     * The name created by alloc instruction
                     * can have any value. Name is only declared
                     * In-SSA form it is not possible to define
                     * this name.
                     */
                case Opcode::PTRADD:
                    /*
                     * In most cases ptr value wouldn't be known.
                     * However, C/C++ allows creating pointer from
                     * literals. However, at this moment there is
                     * not support for ptr. Ptr's are opaque handled
                     * by reference interpreter.
                     */
                case Opcode::LOAD:
                    values[inst->GetDest()] = LVT::INDETERMINABLE;
                    worklist.push_back(inst->GetDest());
                    break;
                case Opcode::CONST:
                    values[inst->GetDest()] = LVT::CONSTANT;
                    constants[inst->GetDest()] = inst->GetOperand(0);
                    worklist.push_back(inst->GetDest());
                    break;
                default:
                    values[inst->GetDest()] = LVT::UNKNOWN;
                    break;
                }
            }
        }
    }
}

void SSCPTransformer::Propagate() {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__ << "\n";
#endif

    for (auto i = 0ul; i < worklist.size(); ++i) {
#ifdef PRINT_DEBUG
        std::cerr << "  Operand: " << worklist[i]->GetName() << "\n";
#endif
        for (auto *use : worklist[i]->GetUses()) {
#ifdef PRINT_DEBUG
            std::cerr << "    Use: ";
            use->Dump(std::cerr);
#endif
            if (use->HasDest() && use->GetOpcode() != Opcode::CALL) {
#ifdef PRINT_DEBUG
                std::cerr << "      HasDest\n";
#endif
                auto *dest = use->GetDest();
                if (values[dest] != LVT::INDETERMINABLE) {
#ifdef PRINT_DEBUG
                    std::cerr << "      Not LVT::INDETERMINABLE\n";
#endif
                    auto val_type = values[dest];
                    use->Visit(propagator.get());
                    if (values[dest] != val_type) {
                        worklist.push_back(dest);
                    }
                }
            } else if (use->GetOpcode() == Opcode::SET) {
                auto *geti = static_cast<SetInstruction *>(use)->GetGetPair();
                geti->Visit(propagator.get());
            }
        }
#ifdef PRINT_DEBUG
        std::cerr << "\n";
#endif
    }
#ifdef PRINT_DEBUG
    std::cerr << "\n";
#endif
}

void SSCPTransformer::Simplify() {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__ << "\n\n";
#endif
    for (auto *blk : func->GetBlocks()) {

#ifdef PRINT_DEBUG
        std::cerr << "Simplifying block: " << blk->GetName() << "\n";
#endif
        for (auto *instr : blk->GetInstructions()) {
#ifdef PRINT_DEBUG
            std::cerr << "  Instr: ";
            instr->Dump(std::cerr);

#endif
            if (instr->HasDest() && instr->GetOpcode() != Opcode::CALL) {
                auto *dest = instr->GetDest();
                if (values[dest] == LVT::CONSTANT) {
#ifdef PRINT_DEBUG
                    std::cerr << "    Constant\n";
#endif

                    if (instr->GetOpcode() == Opcode::GET) {
                        // remove the corresponding set instruction
                        RemoveSetInstruction(
                            static_cast<GetInstruction *>(instr));
                    }
                    // replace with const instruction
                    auto new_instr = std::make_unique<ConstInstruction>();
                    SetOperandAndUse(new_instr.get(), constants[dest]);
                    SetDestAndDef(new_instr.get(), instr->ReleaseDest());
                    blk->AddInstruction(std::move(new_instr),
                                        instr->GetIndex());
                }

            } else if (instr->GetOpcode() == Opcode::BR) {
                auto *br = static_cast<BranchInstruction *>(instr);
                auto *op = instr->GetOperand(0);

                if (values[op] == LVT::CONSTANT) {
#ifdef PRINT_DEBUG
                    std::cerr << "    Constant\n";
#endif
                    auto bool_op = static_cast<BoolOperand *>(constants[op]);
                    auto jmp = std::make_unique<JmpInstruction>();

                    // block to remove
                    Block *rblk;
                    if (bool_op->GetValue()) {
                        jmp->SetJmpDest(br->GetTrueDest());
                        rblk = br->GetFalseDest()->GetBlock();

                    } else {
                        // false
                        jmp->SetJmpDest(br->GetFalseDest());
                        rblk = br->GetTrueDest()->GetBlock();
                    }

                    blk->RemoveSuccessor(rblk);
                    rblk->RemovePredecessor(blk);

                    // Remove the block if can't get to it from any other
                    // block.
                    if (rblk->GetPredecessorSize() == 0) {
                        // Drain the block in reverse.
                        // This will avoid dead reference from
                        // preventing any block being put in the
                        // remove queue.
                        for (auto i :
                             std::views::iota(0ul, rblk->GetSuccessorSize()) |
                                 std::views::reverse) {
                            rblk->GetSuccessor(i)->RemovePredecessor(rblk);
                            rblk->RemoveSuccessor(i);
                        }

                        remove.push_back(
                            br->GetFalseDest()->GetBlock()->GetIndex());
                    }

                    blk->AddInstruction(std::move(jmp), instr->GetIndex());
                }
            }
        }
#ifdef PRINT_DEBUG
        std::cerr << "\n";
#endif
    }

    if (remove.size()) {
        SimplifyCFG();
    }
}

void SSCPTransformer::SimplifyCFG() {
    for (auto idx : remove) {
        auto *blk = func->GetBlock(idx);
        for (auto *inst : blk->GetInstructions()) {
            if (inst->GetOpcode() == Opcode::GET) {
                RemoveSetInstruction(static_cast<GetInstruction *>(inst));
            }
        }
    }

    func->RemoveBlocks(std::move(remove));
}

void ConstantPropagator::VisitAndInstruction(AndInstruction *instr) {
    auto *dest = instr->GetDest();
    auto *op0 = instr->GetOperand(0);
    auto *op1 = instr->GetOperand(1);

    if (sscp->values[op0] == LVT::INDETERMINABLE ||
        sscp->values[op1] == LVT::INDETERMINABLE) {
        sscp->values[dest] = LVT::INDETERMINABLE;
        return;
    }

    if (sscp->values[op0] == LVT::CONSTANT &&
        sscp->values[op1] == LVT::CONSTANT) {
        auto value =
            static_cast<BoolOperand *>(sscp->constants[op0])->GetValue() &&
            static_cast<BoolOperand *>(sscp->constants[op1])->GetValue();

        sscp->values[dest] = LVT::CONSTANT;
        sscp->constants[dest] = BoolOperand::GetOperand(value);
        return;
    }

    assert(sscp->values[op0] == LVT::CONSTANT ||
           sscp->values[op1] == LVT::CONSTANT);

    auto *const_op = op0;
    if (sscp->values[op1] == LVT::CONSTANT) {
        const_op = op1;
    }

    if (static_cast<BoolOperand *>(sscp->constants[const_op])->GetValue() ==
        false) {
        sscp->values[dest] = LVT::CONSTANT;
        sscp->constants[dest] = BoolOperand::GetOperand(false);
    }
}

void ConstantPropagator::VisitOrInstruction(OrInstruction *instr) {
    auto *dest = instr->GetDest();
    auto *op0 = instr->GetOperand(0);
    auto *op1 = instr->GetOperand(1);

    if (sscp->values[op0] == LVT::INDETERMINABLE ||
        sscp->values[op1] == LVT::INDETERMINABLE) {
        sscp->values[dest] = LVT::INDETERMINABLE;
        return;
    }

    if (sscp->values[op0] == LVT::CONSTANT &&
        sscp->values[op1] == LVT::CONSTANT) {
        auto value =
            static_cast<BoolOperand *>(sscp->constants[op0])->GetValue() ||
            static_cast<BoolOperand *>(sscp->constants[op1])->GetValue();

        sscp->values[dest] = LVT::CONSTANT;
        sscp->constants[dest] = BoolOperand::GetOperand(value);
        return;
    }

    assert(sscp->values[op0] == LVT::CONSTANT ||
           sscp->values[op1] == LVT::CONSTANT);

    auto *const_op = op0;
    if (sscp->values[op1] == LVT::CONSTANT) {
        const_op = op1;
    }

    if (static_cast<BoolOperand *>(sscp->constants[const_op])->GetValue() ==
        true) {
        sscp->values[dest] = LVT::CONSTANT;
        sscp->constants[dest] = BoolOperand::GetOperand(true);
    }
}

void ConstantPropagator::VisitNotInstruction(NotInstruction *instr) {
    auto *dest = instr->GetDest();
    auto *op0 = instr->GetOperand(0);

    if (sscp->values[op0] == LVT::INDETERMINABLE) {
        sscp->values[dest] = LVT::INDETERMINABLE;
        return;
    }

    if (sscp->values[op0] == LVT::CONSTANT) {
        auto value =
            !static_cast<BoolOperand *>(sscp->constants[op0])->GetValue();

        sscp->values[dest] = LVT::CONSTANT;
        sscp->constants[dest] = BoolOperand::GetOperand(value);
        return;
    }
}

void ConstantPropagator::VisitGetInstruction(GetInstruction *instr) {
    auto *dest = instr->GetDest();
    auto *set0 = instr->GetSetPair(0);

    for (auto *seti : instr->GetSetPairs()) {
        auto *op = seti->GetOperand(0);
        auto v = sscp->values[op];

        if (v == LVT::INDETERMINABLE) {
            sscp->values[dest] = LVT::INDETERMINABLE;
            return;
        }

        if (v == LVT::CONSTANT &&
            sscp->constants[set0->GetOperand(0)] != sscp->constants[op]) {
            sscp->values[dest] = LVT::INDETERMINABLE;
            return;
        } else {
            sscp->values[dest] = LVT::UNKNOWN;
            return;
        }
    }

    sscp->values[dest] = LVT::CONSTANT;
    sscp->constants[dest] = sscp->constants[set0->GetOperand(0)];
}
} // namespace sc
