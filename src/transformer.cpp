#include "transformer.hpp"
#include "analyzer.hpp"
#include "block.hpp"
#include "cfg.hpp"
#include "function.hpp"
#include "instruction.hpp"
#include "opcodes.hpp"
#include "operand.hpp"
#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <memory>
#include <ranges>
#include <unordered_set>

namespace sc {

// #define PRINT_DEBUG
#undef PRINT_DEBUG

// #define PRINT_DEBUG_SSA
#undef PRINT_DEBUG_SSA

// EarlyIRTransformer begin
void EarlyIRTransformer::Transform() {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << "Processing function:  " << func->GetName() << "\n";
#endif
    FixIR();
}

void EarlyIRTransformer::FixIR() {
    std::vector<Block *> rb;
    for (size_t i : std::views::iota(0UL, func->GetBlockSize()) |
                        std::views::take(func->GetBlockSize() - 1)) {
        auto *block = func->GetBlock(i);
#ifdef PRINT_DEBUG
        std::cout << __PRETTY_FUNCTION__
                  << " Processing block:  " << block->GetName() << "\n";
#endif
        if (block->GetInstructionSize()) {
            auto *instr = LAST_INSTR(block);
            if (instr->GetOpCode() == OpCode::RET) {
                rb.push_back(block);
            } else if (instr->GetOpCode() != OpCode::BR &&
                       instr->GetOpCode() != OpCode::JMP) {
                auto jmp_instr = std::make_unique<JmpInstruction>();
                auto *nxt_lbl = func->GetBlock(i + 1)->GetLabel();
                jmp_instr->SetOperand(nxt_lbl);
                func->GetBlock(i)->AddInstruction(std::move(jmp_instr));
            }
        } else {
            auto jmp_instr = std::make_unique<JmpInstruction>();
            auto *nxt_lbl = func->GetBlock(i + 1)->GetLabel();
            jmp_instr->SetOperand(nxt_lbl);
            func->GetBlock(i)->AddInstruction(std::move(jmp_instr));
        }
    }

    auto *block = LAST_BLK(func);
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Processing block:  " << block->GetName() << "\n";
#endif
    if (block->GetInstructionSize() &&
        LAST_INSTR(block)->GetOpCode() == OpCode::RET) {
        rb.push_back(block);
    } else if (!block->GetInstructionSize() || !rb.size()) {
        // can only add a ret instr if function is void
        assert(func->GetRetType() == DataType::VOID &&
               "Non-void function with no return instruction.\n");
        block->AddInstruction(std::make_unique<RetInstruction>());
        rb.push_back(block);
    }

    // last block can only end in these instructions
    assert(LAST_INSTR(block)->GetOpCode() == OpCode::RET ||
           LAST_INSTR(block)->GetOpCode() == OpCode::JMP ||
           LAST_INSTR(block)->GetOpCode() == OpCode::BR);

    if (rb.size() > 1) {
        AddUniqueExitBlock(std::move(rb));
    }
}

void EarlyIRTransformer::AddUniqueExitBlock(std::vector<Block *> rb) {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << "Processing function:  " << func->GetName() << "\n";
#endif
    // add new exit block
    func->AddBlock(std::make_unique<Block>("__sc_exit__"));
    LAST_BLK(func)->SetIndex(func->GetBlockSize() - 1);

    // create new lbl operand to store jmp dest
    auto lbl_op = std::make_unique<LabelOperand>("__sc_exit__");
    lbl_op->SetBlock(LAST_BLK(func));
    LAST_BLK(func)->SetLabel(lbl_op.get());

    // create new ret instruction
    auto ret_instr = std::make_unique<RetInstruction>();
    // create new temp ret operand
    std::unique_ptr<OperandBase> ret_op = nullptr;
    // only add operand if not void
    if (func->GetRetType() != DataType::VOID) {
        if (func->GetRetType() == DataType::PTR) {
            ret_op = std::make_unique<PtrOperand>("__rret__");
            for (auto dt : static_cast<PtrFunction *>(func)->GetPtrChain()) {
                static_cast<PtrOperand *>(ret_op.get())->AppendPtrChain(dt);
            }
        } else {
            ret_op =
                std::make_unique<RegOperand>(func->GetRetType(), "__rret__");
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
// EarlyIRTransformer end

// CFTransformer begin
void CFTransformer::Transform() {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Processing Function:  " << f->GetName() << "\n";
#endif
    do {
        traverse_order = GetPostOrder(func);
    } while (Clean());

    // Remove all unreachable nodes
    traverse_order = GetPostOrder(func);
    if (traverse_order.size() != func->GetBlockSize()) {
        RemoveUnreachableCFNode();
    }
}

bool CFTransformer::Clean() {
    bool ret = false;
    for (auto *block : traverse_order) {
#ifdef PRINT_DEBUG
        std::cout << __PRETTY_FUNCTION__ << " Clean:  " << block->GetName()
                  << "\n";
#endif
        auto instr = LAST_INSTR(block);
        if (instr->GetOpCode() == OpCode::BR &&
            instr->GetOperand(0) == instr->GetOperand(1)) {
            ReplaceBrWithJmp(block);
            ret = true;
        }
        if (instr->GetOpCode() == OpCode::JMP) {
            // Do not remove the block if it's the entry block
            // unique entry block! Otherwise there won't be an
            // unique exit block in reverse CFG
            if (block->GetInstructionSize() == 1 &&
                block->GetPredecessorSize()) {
                // Empty block one label and one jmp
                RemoveEmptyBlock(block);
                ret = true;
            }
            // exit node has no successor
            auto *succ_blk =
                block->GetSuccessorSize() ? block->GetSuccessor(0) : nullptr;
            if (succ_blk && succ_blk->GetPredecessorSize() == 1) {
                // combine current and next block
                CombineBlocks(block);
                ret = true;
            }
            if (succ_blk && succ_blk->GetInstructionSize() == 1 &&
                LAST_INSTR(succ_blk)->GetOpCode() == OpCode::BR) {
                // change block jmp with succ_blk br
                HoistBranch(block);
                ret = true;
            }
        }
    }
    return ret;
}

void CFTransformer::ReplaceBrWithJmp(Block *block) {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Replacing branch with jmp in: " << block->GetName() << "\n";
#endif
    auto jmp_instr = std::make_unique<JmpInstruction>();
    jmp_instr->SetOperand(LAST_INSTR(block)->GetOperand(0));
    block->AddInstruction(std::move(jmp_instr),
                          block->GetInstructionSize() - 1);
    block->RemoveSuccessor(block->GetSuccessorSize() - 1);
    auto *succ_blk = block->GetSuccessor(0);

    // The predecessor block should ideally have two predecessor link to current
    // block. Remove the other reference.
    size_t count = 0;
    for (size_t i : std::views::iota(0UL, succ_blk->GetPredecessorSize())) {
        if (succ_blk->GetPredecessor(i) == block) {
            ++count;
            if (count > 1) {
                succ_blk->RemovePredecessor(i);
            }
        }
    }
}

void CFTransformer::RemoveEmptyBlock(Block *block) {
#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__ << " Removing block: " << block->GetName()
              << "\n";
#endif
    auto *succ_blk = block->GetSuccessor(0);
    RemovePredecessorIf(succ_blk, block);

    // remove the successor since clean is
    // implemented as a series of if's
    RemoveSuccessorIf(block, succ_blk);

    for (size_t i : std::views::iota(0UL, block->GetPredecessorSize())) {
        auto pred_blk = block->GetPredecessor(i);
        auto lst_instr = LAST_INSTR(pred_blk);
        if (lst_instr->GetOpCode() == OpCode::JMP) {
            lst_instr->SetOperand(succ_blk->GetLabel(), 0);
            pred_blk->AddSuccessor(succ_blk, 0);
        } else {
            assert(lst_instr->GetOpCode() == OpCode::BR);
            if (static_cast<LabelOperand *>(lst_instr->GetOperand(0))
                    ->GetBlock() == block) {
                lst_instr->SetOperand(succ_blk->GetLabel(), 0);
                pred_blk->AddSuccessor(succ_blk, 0);
            }

            if (static_cast<LabelOperand *>(lst_instr->GetOperand(1))
                    ->GetBlock() == block) {
                lst_instr->SetOperand(succ_blk->GetLabel(), 1);
                pred_blk->AddSuccessor(succ_blk, 1);
            }
        }
        succ_blk->AddPredecessor(pred_blk);
    }

    for (size_t i : std::views::iota(0UL, block->GetPredecessorSize()) |
                        std::views::reverse) {
        block->RemovePredecessor(i);
    }
}

void CFTransformer::CombineBlocks(Block *block) {
    // remove the jmp instruction from the block
    block->RemoveInstruction(block->GetInstructionSize() - 1);

    // Add succ blocks instr into current block
    auto *succ_blk = block->GetSuccessor(0);

#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Combining block: " << succ_blk->GetName()
              << " into: " << block->GetName() << "\n";
#endif

    auto instructions = succ_blk->ReleaseInstructions();
    for (size_t i : std::views::iota(0UL, instructions.size())) {
        block->AddInstruction(std::move(instructions[i]));
    }

    block->RemoveSuccessor(0);

    for (size_t i : std::views::iota(0UL, succ_blk->GetSuccessorSize())) {
        RemovePredecessorIf(succ_blk->GetSuccessor(i), succ_blk);
        succ_blk->GetSuccessor(i)->AddPredecessor(block);
        block->AddSuccessor(succ_blk->GetSuccessor(i));
    }
}

void CFTransformer::HoistBranch(Block *block) {
    auto *succ_blk = block->GetSuccessor(0);
    RemovePredecessorIf(succ_blk, block);
    RemoveSuccessorIf(block, succ_blk);

#ifdef PRINT_DEBUG
    std::cout << __PRETTY_FUNCTION__
              << " Hoisting branch from: " << succ_blk->GetName()
              << " to: " << block->GetName() << "\n";
#endif

    auto *last_instr = LAST_INSTR(succ_blk);
    auto br_instr = std::make_unique<BranchInstruction>();
    for (size_t i : std::views::iota(0UL, 2UL)) {
        auto *op = static_cast<LabelOperand *>(last_instr->GetOperand(i));
        br_instr->SetOperand(op);
        block->AddSuccessor(op->GetBlock());
        op->GetBlock()->AddPredecessor(block);
    }
    br_instr->SetOperand(last_instr->GetOperand(2));

    block->AddInstruction(std::move(br_instr), block->GetInstructionSize() - 1);
}

void CFTransformer::RemoveUnreachableCFNode() {
    std::unordered_set<Block *> reachable(traverse_order.cbegin(),
                                          traverse_order.cend());
    auto &blocks = func->GetBlocks();
    blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                                [&reachable](auto &block) {
                                    if (!reachable.contains(block.get())) {
#ifdef PRINT_DEBUG
                                        std::cout << __PRETTY_FUNCTION__
                                                  << " Removing block: "
                                                  << block->GetName() << "\n";
#endif
                                        // TODO:
                                        // erase the corresponding LabelOperand
                                        return true;
                                    }
                                    return false;
                                }),
                 blocks.end());

    // fix block indexes
    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        func->GetBlock(i)->SetIndex(i);
    }
}
// CFTransformer end

// SSATransformer begin
void SSATransformer::RewriteInSSAForm() {
#ifdef PRINT_DEBUG_SSA
    std::cout << __PRETTY_FUNCTION__
              << " Processing Function:  " << func->GetName() << "\n\n";
#endif
    globals.ComputeGlobalNames();
    dom.ComputeDominanceFrontier();

    for (auto i : std::views::iota(0ul, func->GetBlockSize())) {
        assert(func->GetBlock(i)->GetIndex() == i);
    }

    for (auto *op : globals.GetGlobals()) {
#ifdef PRINT_DEBUG_SSA
        std::cout << "Global: " << op->GetName() << "\n\n";
#endif
        auto gblocks = globals.GetBlocks(op);
        auto worklist = std::vector<Block *>(gblocks.begin(), gblocks.end());
        auto wl_size = worklist.size();
        for (size_t i = 0; i < wl_size; ++i) {
            auto *block = worklist[i];
#ifdef PRINT_DEBUG_SSA
            std::cout << "  Worklist: " << block->GetName() << "\n\n";
#endif

            for (auto *d : dom.GetDominanceFrontier(block)) {
                // add get instruction
                if (gets[d->GetIndex()].contains(op)) {
                    continue;
                }

#ifdef PRINT_DEBUG_SSA
                std::cout << "\t" << d->GetName() << ": " << op->GetName()
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
#ifdef PRINT_DEBUG_SSA
                    std::cout << "\t" << pred->GetName() << ": set "
                              << op->GetName() << "\n";
#endif
                }

                // Insert the get instruction
                d->InsertInstruction(std::move(get_instr), 0ul);

#ifdef PRINT_DEBUG_SSA
                std::cout << "\n";
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

#ifdef PRINT_DEBUG_SSA
    std::cout << "\n";
#endif

    if (func->GetArgsSize()) {
        for (auto i : std::views::iota(0ul, func->GetArgsSize())) {
            name[func->GetArgs(i)].push(func->GetArgs(i));
        }
    }

    dom.BuildDominatorTree();
    RenameGet(func->GetBlock(0));
}

void SSATransformer::RenameGet(Block *block) {
#ifdef PRINT_DEBUG_SSA
    std::cout << __PRETTY_FUNCTION__
              << "Processing Block:  " << block->GetName() << "\n";
#endif

    std::unordered_map<OperandBase *, size_t> pop_count;

    auto process = [this, &pop_count](InstructionBase *instr, size_t start,
                                      bool dest = true) {
#ifdef PRINT_DEBUG_SSA
        std::cout << "\n\tBefore: ";
        instr->Dump();
#endif

        for (auto i : std::views::iota(start, instr->GetOperandSize())) {
            instr->SetOperand(name[instr->GetOperand(i)].top(), i);
        }

        if (dest) {
            pop_count[instr->GetOperand(0)]++;
            auto *ndest = NewDest(instr->GetOperand(0));
            instr->SetOperand(ndest, 0);
        }

#ifdef PRINT_DEBUG_SSA
        std::cout << "\tAfter: ";
        instr->Dump();
#endif
    };

    size_t i = 0;
    for (i = 0; i < block->GetInstructionSize(); ++i) {
        auto *instr = block->GetInstruction(i);
        if (instr->GetOpCode() != OpCode::GET) {
            break;
        }
        process(instr, instr->GetOperandSize(), true);

        auto *geti = static_cast<GetInstruction *>(instr);
        for (auto i : std::views::iota(0ul, geti->GetSetPairSize())) {
            auto *seti = geti->GetSetPair(i);
#ifdef PRINT_DEBUG_SSA
            std::cout << "\n\t\t  Before: ";
            seti->Dump();
#endif

            seti->SetOperand(geti->GetOperand(0), 0);

#ifdef PRINT_DEBUG_SSA
            std::cout << "\n\t\t  After: ";
            seti->Dump();
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
            process(instr, 2, false);
            break;
        case OpCode::GET:
            assert(false);
        case OpCode::SET: {
            if (name[instr->GetOperand(1)].empty()) {
                auto undef_instr = std::make_unique<UndefInstruction>();
                auto *ndest = NewDest(instr->GetOperand(1));
                undef_instr->SetOperand(ndest);
                undef_instr->SetOperand(UndefOperand::GetUndefOperand());
#ifdef PRINT_DEBUG_SSA
                std::cout << "\n\tInsert: ";
                undef_instr->Dump();
#endif
                func->GetBlock(0)->InsertInstruction(std::move(undef_instr), 0);
                if (func->GetBlock(0) == block) {
                    ++i;
                }
            }
            process(instr, 1, false);
        } break;
        case OpCode::CALL: {
            auto *call = static_cast<CallInstruction *>(instr);
            process(instr, call->GetRetVal(), call->GetRetVal());
        } break;
        case OpCode::RET: {
            auto *ret = static_cast<RetInstruction *>(instr);
            if (ret->GetOperandSize()) {
                process(instr, 0, false);
            }
        } break;
        case OpCode::FREE:
        case OpCode::STORE:
        case OpCode::PRINT:
            process(instr, 0, false);
            break;
        case OpCode::CONST:
            process(instr, instr->GetOperandSize(), true);
            break;
        default:
            process(instr, 1, true);
            break;
        }
    }

#ifdef PRINT_DEBUG_SSA
    std::cout << "\n";
#endif

    // dom
    for (auto *succ : dom.GetDTreeSuccessor(block)) {
        RenameGet(succ);
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
// SSATransformer end

// DVNTransformer begin
bool DVNTransformer::IsUseless(GetInstruction *geti) {
    // TODO:
    // How to handle the set instructions??
    // What if the value-number for the set instruction is still not there!

    auto *vn0 = geti->GetSetPair(0)->GetOperand(1);

    // TODO:
    // Remove workaround
    if(!vn0) {
        return false;
    }

    for (auto i : std::views::iota(1ul, geti->GetSetPairSize())) {
        auto *vn = geti->GetSetPair(i)->GetOperand(1);

        // TODO:
        // remove workaround
        if(!vn) {
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
            if(oprnd) {
                vt.Insert(instr->GetOperand(0)->GetName(), oprnd);
                remove_instrs[block->GetIndex()].push_back(i);
            } else {
                vt.Insert(instr->GetOperand(0)->GetName(), instr->GetOperand(0));
                vt.Insert(instr->GetOperand(1)->GetName(), instr->GetOperand(0));
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

} // namespace sc
