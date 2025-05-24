#include "transformers/dvn_transformer.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"
#include "operand.hpp"
#include <memory>
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

void DVNTransformer::DVN(Block *block) {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__
              << " Processing Block: " << block->GetName() << "\n";
#endif

    vt.PushScope();

    /*
     * Note: Since we don't consider br instruction the opportunity to
     * simplify the br instruction in case of const argument is lost!
     */

    for (auto i : std::views::iota(0ul, block->GetInstructionSize())) {
        auto *instr = block->GetInstruction(i);

        if (instr->HasDest()) {
#ifdef PRINT_DEBUG
            instr->Dump(std::cerr << "\n  ");
#endif
            auto Opcode = instr->GetOpcode();

            if (Opcode == Opcode::GET) {
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
            } else if (Opcode == Opcode::CALL || Opcode == Opcode::UNDEF ||
                       Opcode == Opcode::ALLOC) {
                /*
                 * Call Instruction can have side-effect
                 */

                // Need to set the value number for the dest
                vt.Insert(instr->GetDest()->GetName(), instr->GetDest());
            } else {
                Process(instr, i);
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

void DVNTransformer::Process(InstructionBase *instr, size_t idx) {
    if (instr->GetOperandSize() == 2) {
        assert(instr->GetOperand(0) == vt.Get(instr->GetOperand(0)->GetName()));
        assert(instr->GetOperand(1) == vt.Get(instr->GetOperand(1)->GetName()));

        // value number left operand
        auto *vn_lop = instr->GetOperand(0);
        auto *vn_rop = instr->GetOperand(1);

        // TODO:
        // This check is a workaround since the function
        // arguments have no defining instructions
        if (vn_lop->GetDef() && vn_rop->GetDef()) {
            if (vn_lop->GetDef()->GetOpcode() == Opcode::CONST &&
                vn_rop->GetDef()->GetOpcode() == Opcode::CONST) {
                instr = FoldConstInstruction(instr, idx);
            }
        }

        // if (CheckIdentity(block, instr, idx)) {
        //     return;
        // }
    } else {
        if (instr->GetOpcode() == Opcode::NOT &&
            instr->GetOperand(0)->GetDef() &&
            instr->GetOperand(0)->GetDef()->GetOpcode() == Opcode::CONST) {
            instr = FoldConstInstruction(instr, idx);
        }
    }

    auto [key, oprnd] = GetKeyAndVN(instr);
    auto *dest = instr->GetDest();

    if (oprnd) {
        vt.Insert(dest->GetName(), oprnd);
        ReplaceUses(dest, oprnd);
        MarkForRemoval(instr->GetBlock(), idx);
    } else {
        vt.Insert(dest->GetName(), dest);
        vt.Insert(key, dest);
    }
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

void DVNTransformer::MarkForRemoval(Block *block, size_t idx) {
    auto *instr = block->GetInstruction(idx);
    for (auto *op : instr->GetOperands()) {
        op->RemoveUse(instr);
    }
    remove_instrs[block->GetIndex()].push_back(idx);
}

InstructionBase *DVNTransformer::FoldConstInstruction(InstructionBase *instr,
                                                      size_t idx) {
    auto *block = instr->GetBlock();
    auto new_inst = std::make_unique<ConstInstruction>();
    SetDestAndDef(new_inst.get(), instr->GetDest());
    auto *op = interpreter.ProcessInstruction(instr);
    SetOperandAndUse(new_inst.get(), op);
    instr = new_inst.get();
    block->AddInstruction(std::move(new_inst), idx);
    return instr;
}

std::pair<std::string, OperandBase *>
DVNTransformer::GetKeyAndVN(InstructionBase *instr) const {
    if (instr->GetOperandSize() == 2) {
        auto *binstr = static_cast<BinaryOperator *>(instr);

        auto key = instr->GetOperand(0)->GetName() +
                   std::to_string(static_cast<int>(instr->GetOpcode())) +
                   instr->GetOperand(1)->GetName();
        auto *vn = vt.Get(key);

        if (!vn) {
            if (binstr->Commutative()) {
                auto okey =
                    instr->GetOperand(1)->GetName() +
                    std::to_string(static_cast<int>(instr->GetOpcode())) +
                    instr->GetOperand(0)->GetName();
                vn = vt.Get(okey);
            } else if (binstr->NegateCommutative()) {
                auto okey = instr->GetOperand(1)->GetName() +
                            std::to_string(static_cast<int>(
                                binstr->NegateCommutativityOp())) +
                            instr->GetOperand(0)->GetName();
                vn = vt.Get(okey);
            }
        }

        return {key, vn};

    } else {
        assert(instr->GetOpcode() == Opcode::CONST ||
               instr->GetOperand(0) == vt.Get(instr->GetOperand(0)->GetName()));
        if (instr->GetOpcode() == Opcode::ID) {
            auto key = instr->GetOperand(0)->GetName();
            return {key, vt.Get(key)};
        } else {
            auto key = std::to_string(static_cast<int>(instr->GetOpcode())) +
                       instr->GetOperand(0)->GetName();
            return {key, vt.Get(key)};
        }
    }
}

void DVNTransformer::RemoveInstructions() {
    for (auto *block : func->GetBlocks()) {
        block->RemoveInstructions(std::move(remove_instrs[block->GetIndex()]));
    }
}

// DVNTransformer end
} // namespace sc
