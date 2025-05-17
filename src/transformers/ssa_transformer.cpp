#include "transformers/ssa_transformer.hpp"
#include <format>
#include <ranges>

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

    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        assert(func->GetBlock(i)->GetIndex() == i);
    }

    for (auto *op : globals.GetGlobals()) {
#ifdef PRINT_DEBUG
        std::cerr << "Global: " << op->GetName() << "\n\n";
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
                get_instr->SetOperand(op);

                // add set instructions
                for (auto *pred : d->GetPredecessors()) {
                    auto set_instr = std::make_unique<SetInstruction>();
                    set_instr->SetOperand(op);
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

    if (func->GetArgsSize()) {
        for (auto i : std::views::iota(0ul, func->GetArgsSize())) {
            name[func->GetArgs(i)].push(func->GetArgs(i));
        }
    }

    dom.BuildDominatorTree();
    Rename(func->GetBlock(0));
}

void SSATransformer::Rename(Block *block) {
#ifdef PRINT_DEBUG
    std::cerr << __PRETTY_FUNCTION__
              << "Processing Block:  " << block->GetName() << "\n";
#endif

    std::unordered_map<OperandBase *, size_t> pop_count;

    size_t i = 0;
    for (i = 0; i < block->GetInstructionSize(); ++i) {
        auto *instr = block->GetInstruction(i);
        if (instr->GetOpCode() != OpCode::GET) {
            break;
        }
        Process(pop_count, instr, instr->GetOperandSize(), true);

        auto *geti = static_cast<GetInstruction *>(instr);
        for (auto i : std::views::iota(0ul, geti->GetSetPairSize())) {
            auto *seti = geti->GetSetPair(i);
#ifdef PRINT_DEBUG
            std::cerr << "\n\t\t  Before: ";
            seti->Dump(std::cerr);
#endif

            seti->SetOperand(geti->GetOperand(0), 0);

#ifdef PRINT_DEBUG
            std::cerr << "\n\t\t  After: ";
            seti->Dump(std::cerr);
#endif
        }
    }

    for (; i < block->GetInstructionSize(); ++i) {
        auto *instr = block->GetInstruction(i);

        auto opcode = instr->GetOpCode();
        switch (opcode) {
        case OpCode::JMP:
        case OpCode::LABEL:
        case OpCode::NOP:
            // Don't do anything
            break;
        case OpCode::BR:
            Process(pop_count, instr, 2, false);
            break;
        case OpCode::GET:
            assert(false);
        case OpCode::SET: {
            if (name[instr->GetOperand(1)].empty()) {
                auto undef_instr = std::make_unique<UndefInstruction>();
                auto *ndest = NewDest(instr->GetOperand(1));
                undef_instr->SetOperand(ndest);
                undef_instr->SetOperand(UndefOperand::GetUndefOperand());
#ifdef PRINT_DEBUG
                std::cerr << "\n\tInsert: ";
                undef_instr->Dump(std::cerr);
#endif
                func->GetBlock(0)->InsertInstruction(std::move(undef_instr), 0);
                if (func->GetBlock(0) == block) {
                    ++i;
                }
            }
            Process(pop_count, instr, 1, false);
        } break;
        case OpCode::CALL: {
            auto *call = static_cast<CallInstruction *>(instr);
            Process(pop_count, instr, call->GetRetVal(), call->GetRetVal());
        } break;
        case OpCode::RET: {
            auto *ret = static_cast<RetInstruction *>(instr);
            if (ret->GetOperandSize()) {
                Process(pop_count, instr, 0, false);
            }
        } break;
        case OpCode::FREE:
        case OpCode::STORE:
        case OpCode::PRINT:
            Process(pop_count, instr, 0, false);
            break;
        case OpCode::CONST:
            Process(pop_count, instr, instr->GetOperandSize(), true);
            break;
        default:
            Process(pop_count, instr, 1, true);
            break;
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

OperandBase *SSATransformer::NewDest(OperandBase *op) {
    // TODO:
    // cleanup old operands that are not
    // used for SSA transformation
    auto i = counter[op]++;
    auto nop = op->Clone();
    name[op].push(nop.get());

    nop->SetName(std::format("{}.{}", op->GetName(), i));
    func->AddOperand(std::move(nop));

    return name[op].top();
}

void SSATransformer::Process(
    std::unordered_map<OperandBase *, size_t> &pop_count,
    InstructionBase *instr, size_t start, bool dest) {
#ifdef PRINT_DEBUG
    std::cerr << "\n\tBefore: ";
    instr->Dump(std::cerr);
#endif

    for (auto i : std::views::iota(start, instr->GetOperandSize())) {
        auto op = name[instr->GetOperand(i)].top();
        op->SetUse(instr);
        instr->SetOperand(op, i);
    }

    if (dest) {
        pop_count[instr->GetOperand(0)]++;
        auto *ndest = NewDest(instr->GetOperand(0));
        ndest->SetDef(instr);
        instr->SetOperand(ndest, 0);
    }

#ifdef PRINT_DEBUG
    std::cerr << "\tAfter: ";
    instr->Dump(std::cerr);
#endif
}

// SSATransformer end
} // namespace sc
