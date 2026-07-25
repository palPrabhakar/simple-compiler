#include "transformers/ssa_transformer.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"
#include "operand.hpp"
#include <format>
#include <memory>
#include <ranges>
#include <unordered_map>

// #define PRINT_DEBUG

namespace sc {
// SSAInTransformer begin
void SSAInTransformer::RewriteInSSAForm() {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__
              << " Processing Function:  " << func->GetName() << "\n\n";
#endif
    globals.ComputeGlobalNames();
    dom.ComputeDominanceFrontier();

    for (auto *op : globals.GetGlobals()) {
#ifdef PRINT_DEBUG
        std::cerr << "Global: " << op->GetName() << "\n" << std::endl;
#endif
        auto gblocks = globals.GetBlocks(op);
        auto worklist = std::vector<Block *>(gblocks.begin(), gblocks.end());
        auto wl_size = worklist.size();
        for (size_t i = 0; i < wl_size; ++i) {
            auto *block = worklist[i];
#ifdef PRINT_DEBUG
            std::cerr << "  Worklist: " << block->GetName() << "\n\n";
#endif

            for (auto *d : dom.GetDominanceFrontier(block)) {
                // add get instruction
                if (gets[d->GetIndex()].contains(op)) {
                    continue;
                }

#ifdef PRINT_DEBUG
                std::cerr << "\t" << d->GetName() << ": " << op->GetName()
                          << " = get;\n";
#endif

                gets[d->GetIndex()].insert(op);
                auto get_instr = std::make_unique<GetInstruction>();
                // the destination will be defined
                // during renaming
                get_instr->SetShadow(op);

                // add set instructions
                for (auto *pred : d->GetPredecessors()) {
                    auto set_instr = std::make_unique<SetInstruction>();
                    set_instr->SetShadow(op);
                    set_instr->SetOperand(op);

                    set_instr->SetGetPair(get_instr.get());
                    get_instr->SetSetPair(set_instr.get());

                    // insert set instruction before block
                    // terminating instruction
                    pred->InsertInstruction(std::move(set_instr),
                                            pred->GetInstructionSize() - 1);
#ifdef PRINT_DEBUG
                    std::cerr << "\t" << pred->GetName() << ": set "
                              << op->GetName() << "\n";
#endif
                }

                // Insert the get instruction
                d->InsertInstruction(std::move(get_instr), 0ul);

#ifdef PRINT_DEBUG
                std::cerr << "\n";
#endif

                if (std::find(worklist.begin(), worklist.end(), d) ==
                    worklist.end()) {
                    worklist.push_back(d);
                    ++wl_size;
                }
            }
        }
        counter[op] = 0;
    }

#ifdef PRINT_DEBUG
    std::cerr << "\n";
#endif

    dom.BuildDominatorTree();
    Rename(func->GetBlock(0));
}

void SSAInTransformer::Rename(Block *block) {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__
              << " Processing Block:  " << block->GetName() << "\n";
#endif

    std::unordered_map<OperandBase *, size_t> pop_count;

    for (size_t i = 0; i < block->GetInstructionSize(); ++i) {
        auto *instr = block->GetInstruction(i);
        auto opcode = instr->GetOpcode();

        if (opcode == Opcode::GET) {
            Process(pop_count, instr);

            auto *geti = static_cast<GetInstruction *>(instr);
            for (auto *seti : geti->GetSetPairs()) {
#ifdef PRINT_DEBUG
                std::cerr << "\n\t\t  Before: ";
                seti->Dump(std::cerr);
#endif

                seti->SetShadow(geti->GetDest());

#ifdef PRINT_DEBUG
                std::cerr << "\t\t  After: ";
                seti->Dump(std::cerr);
#endif
            }
        } else if (opcode == Opcode::SET) {
            if (name[instr->GetOperand(0)].empty()) {
                auto undef_instr = std::make_unique<UndefInstruction>();
                SetDestAndDef(undef_instr.get(), NewDest(instr->GetOperand(0)));
                SetOperandAndUse(undef_instr.get(),
                                 UndefOperand::GetUndefOperand().get());

#ifdef PRINT_DEBUG
                std::cerr << "\n\tInsert: ";
                undef_instr->Dump(std::cerr);
#endif
                size_t insert_pos = func->HasArgs() ? func->GetArgsSize() : 0;
                func->GetEntryBlock()->InsertInstruction(std::move(undef_instr),
                                                         insert_pos);
                if (func->GetEntryBlock() == block) {
                    ++i;
                }
            }
            Process(pop_count, instr);
        } else {
            Process(pop_count, instr);
        }
    }

#ifdef PRINT_DEBUG
    std::cerr << "\n";
#endif

    // dom
    for (auto *succ : dom.GetDTreeSuccessor(block)) {
        Rename(succ);
    }

    for (auto [k, v] : pop_count) {
        for (auto _ : std::views::iota(0ul, v)) {
            name[k].pop();
        }
    }
}

std::shared_ptr<OperandBase> SSAInTransformer::NewDest(OperandBase *op) {
    auto i = counter[op]++;
    auto nop = op->Clone(func->ctx.get_next_id());
    name[op].push(nop.get());
    nop->SetName(std::format("{}.{}", op->GetName(), i));
    return nop;
}

void SSAInTransformer::Process(
    std::unordered_map<OperandBase *, size_t> &pop_count,
    InstructionBase *instr) {
#ifdef PRINT_DEBUG
    std::cerr << "\n\tBefore: ";
    instr->Dump(std::cerr);
#endif

    for (auto i : std::views::iota(0u, instr->GetOperandSize())) {
        if (name.contains(instr->GetOperand(i))) {
            auto op = name[instr->GetOperand(i)].top();
            SetOperandAndUse(instr, op, i);
        } else {
            assert(instr->GetOpcode() == Opcode::RET ||
                   instr->GetOpcode() == Opcode::CALL ||
                   instr->GetOpcode() == Opcode::CONST);
        }
    }

    if (instr->HasDest()) {
        if (instr->GetOpcode() == Opcode::GET) {
            auto *op = static_cast<GetInstruction *>(instr)->GetShadow();
            pop_count[op]++;
            SetDestAndDef(instr, NewDest(op));
        } else {
            temp_store.insert(instr->CopyDest());
            pop_count[instr->GetDest()]++;
            SetDestAndDef(instr, NewDest(instr->GetDest()));
        }
    }

#ifdef PRINT_DEBUG
    std::cerr << "\tAfter: ";
    instr->Dump(std::cerr);
#endif
}
// SSAInTransformer end

// SSAOutTransformer begin
void SSAOutTransformer::InsertCopies() {
    // If a block has more than one set instructions
    // set by the same variable then need to give
    // different name to them to maintain the ssa property
    std::unordered_map<std::string, int> seen;

    // iterate over all blocks
    for (Block *blk : func->GetBlocks()) {
        // iterate over all intructions in the blocks
        for (size_t i = 0; i < blk->GetInstructionSize(); ++i) {
            InstructionBase *inst = blk->GetInstruction(i);

            // find get instructions
            if (inst->GetOpcode() == Opcode::GET) {
                // rename get variable (a -> a')
                GetInstruction *geti = static_cast<GetInstruction *>(inst);
                std::shared_ptr<OperandBase> dest = geti->ReleaseDest();
                std::shared_ptr<OperandBase> new_dest = dest->Clone(func->ctx.get_next_id());
                new_dest->SetName(std::format("_d{}_", dest->GetName()));
                SetDestAndDef(geti, std::move(new_dest));

                // add mov insruction a = a' after get instruction
                // might not be the most optimum way of inserting
                // instructions when there are multiple get instr
                auto copy_ssa = std::make_unique<IdInstruction>();
                SetDestAndDef(copy_ssa.get(), std::move(dest));
                SetOperandAndUse(copy_ssa.get(), geti->GetDest());
                blk->InsertInstruction(std::move(copy_ssa), i + 1);

                // find corresponding set instruction
                for (size_t i : std::views::iota(0u, geti->GetSetPairSize())) {
                    // rename set shadow variable (a -> a')
                    SetInstruction *seti = geti->GetSetPair(i);
                    seti->SetShadow(geti->GetDest());
                    OperandBase *oprnd = seti->GetOperand(0);

                    // insert mov instruction a'i = ai before set
                    auto copy_inst = std::make_unique<IdInstruction>();
                    std::shared_ptr<OperandBase> new_dest = oprnd->Clone(func->ctx.get_next_id());
                    new_dest->SetName(std::format("_d{}_{}_", oprnd->GetName(),
                                                  seen[oprnd->GetName()]++));
                    SetDestAndDef(copy_inst.get(), std::move(new_dest));
                    SetOperandAndUse(copy_inst.get(), oprnd);

                    SetOperandAndUse(seti, copy_inst->GetDest(), 0);

                    Block *sblk = seti->GetBlock();
                    size_t idx = seti->GetIndex();
                    sblk->InsertInstruction(std::move(copy_inst), idx);
                }
            }
        }
    }
}

void SSAOutTransformer::Rename() {
    for(auto *blk : func->GetBlocks()) {
        // iterate over all intructions in the blocks
        for (size_t i = 0; i < blk->GetInstructionSize(); ++i) {
            auto *instr = blk->GetInstruction(i);

            if (instr->GetOpcode() == Opcode::GET) {
                auto *geti = static_cast<GetInstruction *>(instr);

                for(auto *seti : geti->GetSetPairs()) {
                    auto *oprnd = seti->GetOperand(0);
                    auto *def = oprnd->GetDef();
                    // don't need to remove it's usage because operand
                    // that it this instruction uses is also deleted
                    seti->GetBlock()->RemoveInstruction(seti->GetIndex());
                    def->AddDest(geti->CopyDest());
                }

                blk->RemoveInstruction(geti->GetIndex());
            }
        }
    }
}
// SSAOutTransformer end
} // namespace sc
