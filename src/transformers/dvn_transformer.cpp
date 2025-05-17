#include "transformers/dvn_transformer.hpp"
#include "operand.hpp"
#include <ranges>

namespace sc {

// #define PRINT_DEBUG
#undef PRINT_DEBUG

// DVNTransformer begin
bool DVNTransformer::IsUselessOrRedundant(GetInstruction *geti,
                                          std::string &key) {
    std::vector<std::string> keys(func->GetBlockSize());
    bool same = true;
    bool all_null = true;

    auto *test = vt.Get(geti->GetSetPair(0)->GetOperand(1)->GetName());
    for (auto i : std::views::iota(0ul, geti->GetSetPairSize())) {
        auto *seti = geti->GetSetPair(i);
        auto *op = vt.Get(seti->GetOperand(1)->GetName());

        if (op != test) {
            same = false;
        }

        if (!op) {
            // If the value number is not set
            keys[seti->GetBlock()->GetIndex()] = seti->GetOperand(1)->GetName();
        } else {
            all_null = false;
            keys[seti->GetBlock()->GetIndex()] = op->GetName();
        }
    }

    if (!all_null && same) {
        auto *op = vt.Get(geti->GetSetPair(0)->GetOperand(1)->GetName());
        ReplaceUses(geti->GetOperand(0), op);
#ifdef PRINT_DEBUG
        std::cerr << "    Useless Get:\n";
#endif
        return true;
    }

    std::ranges::for_each(keys, [&key](std::string name) { key += name; });

    auto *op = vt.Get(key, true);
    if (op) {
#ifdef PRINT_DEBUG
        std::cerr << "    Redundant Get: " << op->GetName() << "\n";
#endif
        vt.Insert(geti->GetOperand(0)->GetName(), op);
        ReplaceUses(geti->GetOperand(0), op);
        return true;
    }

    return false;
}

void DVNTransformer::DVN(Block *block) {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__
              << " Processing Block: " << block->GetName() << "\n";
#endif

    vt.PushScope();

    // TODO:
    // Clean the code using the def-uses chain
    // Only need to consider instructions that define a value
    // Use the def-uses chain to update the rest

    // TODO:
    // Fix def-use chain
    // Clean-up and simplify

    for (auto i : std::views::iota(0ul, block->GetInstructionSize())) {
        auto *instr = block->GetInstruction(i);

#ifdef PRINT_DEBUG
        instr->Dump(std::cerr << "\n  ");
#endif
        auto opcode = instr->GetOpCode();
        switch (opcode) {
        case OpCode::JMP:
        case OpCode::LABEL:
        case OpCode::NOP:
        case OpCode::BR:
        case OpCode::SET:
        case OpCode::RET:
        case OpCode::FREE:
        case OpCode::STORE:
        case OpCode::PRINT:
            break;
        case OpCode::GET: {
            // check if all set pair set the same value
            auto *geti = static_cast<GetInstruction *>(instr);
            bool removei = false;
            std::string key = "";

            if (IsUselessOrRedundant(geti, key)) {
                removei = true;
            } else {
                // No need to replace the uses in this case since
                // the value number is same as the operand defined
                vt.Insert(geti->GetOperand(0)->GetName(), geti->GetOperand(0));
                vt.Insert(key, geti->GetOperand(0));
            }

            if (removei) {
                remove_instrs[block->GetIndex()].push_back(i);

#ifdef PRINT_DEBUG
                std::cerr << "    Removing Instruction\n";
#endif

                // remove corresponding set instr
                for (auto i : std::views::iota(0ul, geti->GetSetPairSize())) {
                    auto *seti = geti->GetSetPair(i);
                    auto *sblk = seti->GetBlock();
                    for (auto j :
                         std::views::iota(0ul, sblk->GetInstructionSize()) |
                             std::views::reverse) {
                        if (sblk->GetInstruction(j) == seti) {
#ifdef PRINT_DEBUG
                            std::cerr << "    Removing from " << sblk->GetName()
                                      << ": ";
                            seti->Dump(std::cerr);

#endif
                            remove_instrs[sblk->GetIndex()].push_back(j);
                        }
                    }
                }
            }
        } break;
        case OpCode::CALL: {
            auto *call = static_cast<CallInstruction *>(instr);
            size_t start = call->GetRetVal();
            // since function calls can have side-effects
            if (start) {
                vt.Insert(call->GetOperand(0)->GetName(), call->GetOperand(0));
            }

        } break;
        case OpCode::CONST: {
            auto *oprnd = vt.Get(instr->GetOperand(1)->GetName());
            if (oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                ReplaceUses(instr->GetOperand(0), oprnd);
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
            auto key = GetKey(instr);
            auto *oprnd = vt.Get(key);
            if (oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                ReplaceUses(instr->GetOperand(0), oprnd);
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
            auto key = std::to_string(static_cast<int>(instr->GetOpCode())) +
                       instr->GetOperand(1)->GetName();
            auto *oprnd = vt.Get(key);
            if (oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                ReplaceUses(instr->GetOperand(0), oprnd);
                remove_instrs[block->GetIndex()].push_back(i);
            } else {
                vt.Insert(instr->GetOperand(0)->GetName(),
                          instr->GetOperand(0));
                vt.Insert(key, instr->GetOperand(0));
            }
        } break;
        case OpCode::ALLOC: {
            // x = new T[size]
            vt.Insert(instr->GetOperand(0)->GetName(), instr->GetOperand(0));
        } break;
        case OpCode::ID: {
            // x = y;
            vt.Insert(instr->GetOperand(0)->GetName(), instr->GetOperand(1));
            ReplaceUses(instr->GetOperand(0), instr->GetOperand(1));
            remove_instrs[block->GetIndex()].push_back(i);
        } break;
        case OpCode::ADD:
        case OpCode::MUL:
        case OpCode::FADD:
        case OpCode::FMUL: {
            // TODO:
            // commutivity, identities
            // commute

            auto key = GetKey(instr);
            auto *oprnd = vt.Get(key);
            if (oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                ReplaceUses(instr->GetOperand(0), oprnd);
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

#ifdef PRINT_DEBUG
    std::cerr << "\n";
#endif

    for (auto blk : dom.GetDTreeSuccessor(block)) {
        DVN(blk);
    }

    vt.PopScope();
}

void DVNTransformer::RemoveInstructions() {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__
              << " Processing Function: " << func->GetName() << "\n";
#endif
    for (auto bi : std::views::iota(0ul, func->GetBlockSize())) {
        auto blk = func->GetBlock(bi);

#ifdef PRINT_DEBUG
        std::cerr << "  Block: " << blk->GetName() << "\n";
#endif
        for (auto i : std::ranges::reverse_view(remove_instrs[bi])) {

#ifdef PRINT_DEBUG
            blk->GetInstruction(i)->Dump(std::cerr << "    Removing: ");
#endif

            blk->RemoveInstruction(i);
        }

#ifdef PRINT_DEBUG
        std::cerr << "\n";
#endif
    }
}
// DVNTransformer end
}; // namespace sc
