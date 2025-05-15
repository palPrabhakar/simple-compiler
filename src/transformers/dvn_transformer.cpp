#include "transformers/dvn_transformer.hpp"
#include <ranges>

namespace sc {
// DVNTransformer begin
bool DVNTransformer::IsUseless(GetInstruction *geti) {
    // TODO:
    // How to handle the set instructions??
    // What if the value-number for the set instruction is still not there!

    auto *vn0 = geti->GetSetPair(0)->GetOperand(1);

    // TODO:
    // Remove workaround
    if (!vn0) {
        return false;
    }

    for (auto i : std::views::iota(1ul, geti->GetSetPairSize())) {
        auto *vn = geti->GetSetPair(i)->GetOperand(1);

        // TODO:
        // remove workaround
        if (!vn) {
            return false;
        }

        if (vn != vn0) {
            return false;
        }
    }

    vt.Insert(geti->GetOperand(0)->GetName(),
              vt.Get(geti->GetSetPair(0)->GetOperand(1)->GetName()));

    return true;
}

bool DVNTransformer::IsRedundant(GetInstruction *geti) {
    std::vector<std::string> keys(func->GetBlockSize());

    // TODO:
    // Use value number as key or ssa-name??
    // The value number would be same if the key is same
    for (auto i : std::views::iota(0ul, geti->GetSetPairSize())) {
        auto *seti = geti->GetSetPair(i);
        keys[seti->GetBlock()->GetIndex()] = seti->GetOperand(1)->GetName();
    }

    std::string key;
    std::ranges::for_each(keys, [&key](std::string name) { key += name; });

    // need to check only local scope
    auto *oprnd = vt.Get(key, true);

    if (oprnd) {
        vt.Insert(geti->GetOperand(0)->GetName(), oprnd);
        return true;
    }

    return false;
}

void DVNTransformer::DVN(Block *block) {
    vt.PushScope();

    // TODO:
    // Clean the code using the def-uses chain
    // Only need to consider instructions that define a value
    // Use the def-uses chain to update the rest

    for (auto i : std::views::iota(0ul, block->GetInstructionSize())) {
        auto *instr = block->GetInstruction(i);

        auto opcode = instr->GetOpCode();
        switch (opcode) {
        case OpCode::JMP:
        case OpCode::LABEL:
        case OpCode::NOP:
            // Don't do anything
            break;
        case OpCode::BR: {
            auto *oprnd = vt.Get(instr->GetOperand(2)->GetName());
            assert(oprnd);
            instr->SetOperand(oprnd, 2);
        } break;
        case OpCode::GET: {
            // check if all set pair set the same value
            auto *geti = static_cast<GetInstruction *>(instr);
            if (IsUseless(geti) || IsRedundant(geti)) {
                remove_instrs[block->GetIndex()].push_back(i);

                // remove corresponding set instr
                for (auto i : std::views::iota(0ul, geti->GetSetPairSize())) {
                    auto *seti = geti->GetSetPair(i);
                    auto *sblk = seti->GetBlock();
                    for (auto j :
                         std::views::iota(0ul, sblk->GetInstructionSize()) |
                             std::views::reverse) {
                        if (sblk->GetInstruction(j) == seti) {
                            remove_instrs[sblk->GetIndex()].push_back(j);
                        }
                    }
                }
            } else {
                vt.Insert(geti->GetOperand(0)->GetName(), geti->GetOperand(0));
            }
        } break;
        case OpCode::SET: {
            auto *oprnd = vt.Get(instr->GetOperand(1)->GetName());
            assert(oprnd);
            instr->SetOperand(oprnd, 1);
        } break;
        case OpCode::CALL: {
            auto *call = static_cast<CallInstruction *>(instr);
            size_t start = call->GetRetVal();
            for (auto j : std::views::iota(start, call->GetOperandSize())) {
                auto *oprnd = vt.Get(call->GetOperand(j)->GetName());
                assert(oprnd);
                call->SetOperand(oprnd, j);
            }
            // since function calls can have side-effects
            if (start) {
                vt.Insert(call->GetOperand(0)->GetName(), call->GetOperand(0));
            }

        } break;
        case OpCode::RET: {
            auto *ret = static_cast<RetInstruction *>(instr);
            if (ret->GetOperandSize()) {
                auto *oprnd = vt.Get(ret->GetOperand(0)->GetName());
                assert(oprnd);
                ret->SetOperand(oprnd, 0);
            }
        } break;
        case OpCode::FREE:
        case OpCode::STORE:
        case OpCode::PRINT: {
            for (auto j : std::views::iota(0ul, instr->GetOperandSize())) {
                auto *oprnd = vt.Get(instr->GetOperand(j)->GetName());
                assert(oprnd);
                instr->SetOperand(oprnd, j);
            }
        } break;
        case OpCode::CONST: {
            auto oprnd = vt.Get(instr->GetOperand(1)->GetName());
            if (oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                remove_instrs[block->GetIndex()].push_back(i);
            } else {
                vt.Insert(instr->GetOperand(0)->GetName(),
                          instr->GetOperand(0));
                vt.Insert(instr->GetOperand(1)->GetName(),
                          instr->GetOperand(0));
            }
        } break;

        case OpCode::UNDEF: {
            // There is only one unique copy per const or undef
            vt.Insert(instr->GetOperand(0)->GetName(), instr->GetOperand(0));
        } break;
        case OpCode::SUB:
        case OpCode::DIV:
        case OpCode::EQ:
        case OpCode::LT:
        case OpCode::GT:
        case OpCode::LE:
        case OpCode::GE:
        case OpCode::AND: // short-circuit
        case OpCode::OR:  // short-circuit
        case OpCode::PTRADD:
        case OpCode::FSUB:
        case OpCode::FDIV:
        case OpCode::FEQ:
        case OpCode::FLT:
        case OpCode::FGT:
        case OpCode::FLE:
        case OpCode::FGE: {
            // don't commute
            auto *op1 = vt.Get(instr->GetOperand(1)->GetName());
            auto *op2 = vt.Get(instr->GetOperand(2)->GetName());
            assert(op1 && op2);
            instr->SetOperand(op1, 1);
            instr->SetOperand(op2, 2);

            auto key = GetKey(instr);
            auto *oprnd = vt.Get(key);
            if (oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                remove_instrs[block->GetIndex()].push_back(i);
            } else {
                vt.Insert(instr->GetOperand(0)->GetName(),
                          instr->GetOperand(0));
                vt.Insert(key, instr->GetOperand(0));
            }

        } break;
        case OpCode::LOAD:
        case OpCode::NOT: {
            // x = !x;
            auto *op1 = vt.Get(instr->GetOperand(1)->GetName());
            assert(op1);
            instr->SetOperand(op1, 1);
            auto key = std::to_string(static_cast<int>(instr->GetOpCode())) +
                       op1->GetName();
            auto *oprnd = vt.Get(key);
            if (oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                remove_instrs[block->GetIndex()].push_back(i);
            } else {
                vt.Insert(instr->GetOperand(0)->GetName(),
                          instr->GetOperand(0));
                vt.Insert(key, instr->GetOperand(0));
            }
        } break;
        case OpCode::ALLOC: {
            // x = new T[size]
            auto *op1 = vt.Get(instr->GetOperand(1)->GetName());
            assert(op1);
            instr->SetOperand(op1, 1);
            vt.Insert(instr->GetOperand(0)->GetName(), instr->GetOperand(0));
        } break;
        case OpCode::ID: {
            // x = y;
            auto *oprnd = vt.Get(instr->GetOperand(1)->GetName());
            assert(oprnd);
            instr->SetOperand(oprnd, 1);
            vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
            remove_instrs[block->GetIndex()].push_back(i);
        } break;
        case OpCode::ADD:
        case OpCode::MUL:
        case OpCode::FADD:
        case OpCode::FMUL: {
            // TODO:
            // commutivity, identities
            // commute
            auto *op1 = vt.Get(instr->GetOperand(1)->GetName());
            auto *op2 = vt.Get(instr->GetOperand(2)->GetName());
            assert(op1 && op2);
            instr->SetOperand(op1, 1);
            instr->SetOperand(op2, 2);

            auto key = GetKey(instr);
            auto *oprnd = vt.Get(key);
            if (oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                remove_instrs[block->GetIndex()].push_back(i);
            } else {
                vt.Insert(instr->GetOperand(0)->GetName(),
                          instr->GetOperand(0));
                vt.Insert(key, instr->GetOperand(0));
            }
        } break;
        default:
            instr->Dump();
            assert(false);
        }
    }

    // TODO:
    // process in reverse post-order
    for (auto blk : dom.GetDTreeSuccessor(block)) {
        DVN(blk);
    }

    vt.PopScope();
}

void DVNTransformer::RemoveInstructions() {
    for (auto bi : std::views::iota(0ul, func->GetBlockSize())) {
        auto blk = func->GetBlock(bi);
        for (auto i : std::ranges::reverse_view(remove_instrs[bi])) {
            blk->RemoveInstruction(i);
        }
    }
}
// DVNTransformer end
}; // namespace sc
