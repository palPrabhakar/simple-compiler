#include "transformers/dvn_transformer.hpp"
#include "operand.hpp"
#include "gtest/gtest.h"
#include <ranges>

namespace sc {

// #define PRINT_DEBUG
#undef PRINT_DEBUG

// DVNTransformer begin
void DVNTransformer::Transform() {
    if (func->GetArgsSize()) {
        vt.PushScope();
        for (auto *arg : func->GetArgs()) {
            vt.Insert(arg->GetName(), arg);
        }
    }

    dom.BuildRPODominatorTree();
    DVN(func->GetBlock(0));
    RemoveInstructions();
}

bool DVNTransformer::IsUselessOrRedundant(GetInstruction *geti,
                                          std::string &key) {
    std::vector<std::string> keys(func->GetBlockSize());
    bool same = true;
    bool all_null = true;

    // Check the vn of the non-shadow oprnd of the corresponding set pair
    auto *test = vt.Get(geti->GetSetPair(0)->GetOperand(0)->GetName());
    for (auto *seti : geti->GetSetPairs()) {
        auto *op = vt.Get(seti->GetOperand(0)->GetName());

        if (op != test) {
            same = false;
        }

        if (!op) {
            // If the value number is not set
            keys[seti->GetBlock()->GetIndex()] = seti->GetOperand(0)->GetName();
        } else {
            all_null = false;
            keys[seti->GetBlock()->GetIndex()] = op->GetName();
        }
    }

    if (!all_null && same) {
#ifdef PRINT_DEBUG
        std::cerr << "    Useless Get:\n";
#endif
        // The get instruction is useless and it will be removed
        // Replace all the uses of the oprnd defined by this get
        // with the value number of one of it's arguments.
        auto *op = vt.Get(geti->GetSetPair(0)->GetOperand(0)->GetName());
        ReplaceUses(geti->GetDest(), op);
        return true;
    }

    std::ranges::for_each(keys, [&key](std::string name) { key += name; });

    auto *op = vt.Get(key, true);
    if (op) {
#ifdef PRINT_DEBUG
        std::cerr << "    Redundant Get: " << op->GetName() << "\n";
#endif
        auto *dest = geti->GetDest();
        vt.Insert(dest->GetName(), op);
        // The get instruction is redundant and it will be removed
        // Replace all the uses of the oprnd defined by this get
        // with the value number get instr with same key
        ReplaceUses(dest, op);
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

        if (instr->HasDest()) {
#ifdef PRINT_DEBUG
            instr->Dump(std::cerr << "\n  ");
#endif
            auto opcode = instr->GetOpCode();

            switch (opcode) {
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
                    auto *dest = geti->GetDest();
                    vt.Insert(dest->GetName(), dest);
                    vt.Insert(key, dest);
                }

                if (removei) {
                    MarkForRemoval(block, i);

#ifdef PRINT_DEBUG
                    std::cerr << "    Removing Instruction\n";
#endif

                    // remove corresponding set instr
                    for (auto *seti : geti->GetSetPairs()) {
                        seti->GetOperand(0)->RemoveUse(seti);

                        auto *sblk = seti->GetBlock();

                        auto sblki = sblk->GetInstructions();
                        auto it = std::find(sblki.begin(), sblki.end(), seti);
                        assert(it != sblki.end());
                        MarkForRemoval(sblk,
                                       static_cast<size_t>(it - sblki.begin()));
#ifdef PRINT_DEBUG
                        std::cerr << "    Removing from " << sblk->GetName()
                                  << ": ";
                        seti->Dump(std::cerr);
#endif
                    }
                }
            } break;
            case OpCode::CALL: {
                auto *call = static_cast<CallInstruction *>(instr);
                size_t start = call->HasDest();
                // since function calls can have side-effects
                if (start) {
                    vt.Insert(call->GetDest()->GetName(), call->GetDest());
                }

            } break;
            case OpCode::CONST: {
                auto *oprnd = vt.Get(instr->GetOperand(0)->GetName());
                auto *dest = instr->GetDest();
                if (oprnd) {
                    vt.Insert(dest->GetName(), oprnd);
                    ReplaceUses(dest, oprnd);
                    MarkForRemoval(block, i);
                } else {
                    vt.Insert(dest->GetName(), dest);
                    vt.Insert(instr->GetOperand(0)->GetName(), dest);
                }
            } break;
            case OpCode::UNDEF: {
                // There is only one unique copy per const or undef
                vt.Insert(instr->GetDest()->GetName(), instr->GetDest());
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
                auto *dest = instr->GetDest();
                if (oprnd) {
                    vt.Insert(dest->GetName(), oprnd);
                    ReplaceUses(dest, oprnd);
                    MarkForRemoval(block, i);
                } else {
                    vt.Insert(dest->GetName(), dest);
                    vt.Insert(key, dest);
                }

            } break;
            case OpCode::LOAD:
            case OpCode::NOT: {
                // x = !x;
                auto key =
                    std::to_string(static_cast<int>(instr->GetOpCode())) +
                    instr->GetOperand(0)->GetName();
                auto *oprnd = vt.Get(key);
                auto *dest = instr->GetDest();
                if (oprnd) {
                    vt.Insert(dest->GetName(), oprnd);
                    ReplaceUses(dest, oprnd);
                    MarkForRemoval(block, i);
                } else {
                    vt.Insert(dest->GetName(), dest);
                    vt.Insert(key, dest);
                }
            } break;
            case OpCode::ALLOC: {
                // x = new T[size]
                vt.Insert(instr->GetDest()->GetName(), instr->GetDest());
            } break;
            case OpCode::ID: {
                // x = y;
                auto *oprnd = vt.Get(instr->GetOperand(0)->GetName());
                assert(oprnd);
                vt.Insert(instr->GetDest()->GetName(), oprnd);
                ReplaceUses(instr->GetDest(), oprnd);
                MarkForRemoval(block, i);
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
                auto *dest = instr->GetDest();
                if (oprnd) {
                    vt.Insert(dest->GetName(), oprnd);
                    ReplaceUses(dest, oprnd);
                    MarkForRemoval(block, i);
                } else {
                    vt.Insert(dest->GetName(), dest);
                    vt.Insert(key, dest);
                }
            } break;
            default:
                assert(false);
            }
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

void DVNTransformer::MarkForRemoval(Block *block, size_t idx) {
    auto *instr = block->GetInstruction(idx);
    for (auto *op : instr->GetOperands()) {
        op->RemoveUse(instr);
    }
    remove_instrs[block->GetIndex()].push_back(idx);
}

void DVNTransformer::RemoveInstructions() {
    for (auto *block : func->GetBlocks()) {
        block->RemoveInstructions(std::move(remove_instrs[block->GetIndex()]));
    }
}
// DVNTransformer end
} // namespace sc
