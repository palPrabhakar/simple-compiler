#include "analyzers/cfg.hpp"
#include <algorithm>
#include <memory>
#include <ranges>
#include <unordered_set>

namespace sc {

static void PostOrderImpl(Block *block, std::vector<Block *> &post_order,
                          std::unordered_set<Block *> &visited) {
    if (visited.contains(block)) {
        return;
    }

    visited.insert(block);

    for (auto i : std::views::iota(0ul, block->GetSuccessorSize())) {
        PostOrderImpl(block->GetSuccessor(i), post_order, visited);
    }

    post_order.push_back(block);
}

const std::vector<Block *> GetPostOrder(Function *func) {
    std::vector<Block *> post_order;
    std::unordered_set<Block *> visited;
    PostOrderImpl(func->GetBlock(0), post_order, visited);
    return post_order;
}

const std::vector<Block *> GetReversePostOrder(Function *func) {
    auto post_order = GetPostOrder(func);
    std::reverse(post_order.begin(), post_order.end());
    return post_order;
}

static void BuildCFGImpl(Function *func) {
    auto add_nodes = [func](InstructionBase *instr, size_t idx, size_t start,
                            size_t end) {
        for (size_t i : std::views::iota(start, end)) {
            auto *jmp_blk =
                static_cast<LabelOperand *>(instr->GetOperand(i))->GetBlock();
            func->GetBlock(idx)->AddSuccessor(jmp_blk);
            jmp_blk->AddPredecessor(func->GetBlock(idx));
        }
    };
    for (size_t i : std::views::iota(0UL, func->GetBlockSize())) {
        auto instr = LAST_INSTR(func->GetBlock(i));
        switch (instr->GetOpCode()) {
        case OpCode::JMP:
            add_nodes(instr, i, 0, 1);
            break;
        case OpCode::BR:
            add_nodes(instr, i, 0, 2);
            break;
        case OpCode::RET:
            break;
        default:
            assert(false && "FunctionBuild::CFG - Unexpected opcode found\n");
        }
    }
}

std::unique_ptr<Program> BuildCFG(std::unique_ptr<Program> program) {
    for (auto &f : *program) {
        BuildCFGImpl(f.get());
    }
    return program;
}

} // namespace sc
