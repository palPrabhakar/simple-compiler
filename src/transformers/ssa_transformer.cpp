#include "transformers/ssa_transformer.hpp"
#include "instruction.hpp"
#include "operand.hpp"
#include <format>
#include <ranges>

// #define PRINT_DEBUG
#undef PRINT_DEBUG

namespace sc {
// SSATransformer begin
void SSATransformer::RewriteInSSAForm() {
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
                get_instr->SetShadow(op);

                // add set instructions
                for (auto *pred : d->GetPredecessors()) {
                    auto set_instr = std::make_unique<SetInstruction>();
                    set_instr->SetShadow(op);
                    set_instr->SetOperand(op);

                    set_instr->SetGetPair(get_instr.get());
                    get_instr->SetSetPair(set_instr.get());

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

void SSATransformer::Rename(Block *block) {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__
              << "Processing Block:  " << block->GetName() << "\n";
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
                func->GetBlock(0)->InsertInstruction(std::move(undef_instr),
                                                     insert_pos);
                if (func->GetBlock(0) == block) {
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

std::shared_ptr<OperandBase> SSATransformer::NewDest(OperandBase *op) {
    auto i = counter[op]++;
    auto nop = op->Clone();
    name[op].push(nop.get());
    nop->SetName(std::format("{}.{}", op->GetName(), i));
    return nop;
}

void SSATransformer::Process(
    std::unordered_map<OperandBase *, size_t> &pop_count,
    InstructionBase *instr) {
#ifdef PRINT_DEBUG
    std::cerr << "\n\tBefore: ";
    instr->Dump(std::cerr);
#endif

    for (auto i : std::views::iota(0ul, instr->GetOperandSize())) {
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
// SSATransformer end
} // namespace sc
