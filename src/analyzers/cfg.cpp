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

    for (auto *succ : block->GetSuccessors()) {
        PostOrderImpl(succ, post_order, visited);
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
    auto add_nodes = [](Block *pred, Block *succ) {
        pred->AddSuccessor(succ);
        succ->AddPredecessor(pred);
    };
    for (auto *block : func->GetBlocks()) {
        auto *instr = LAST_INSTR(block);
        switch (instr->GetOpcode()) {
        case Opcode::JMP: {
            auto jmp_blk =
                static_cast<JmpInstruction *>(instr)->GetJmpDest()->GetBlock();
            add_nodes(block, jmp_blk);
        } break;
        case Opcode::BR: {
            auto *br = static_cast<BranchInstruction *>(instr);
            add_nodes(block, br->GetTrueDest()->GetBlock());
            add_nodes(block, br->GetFalseDest()->GetBlock());
        } break;
        case Opcode::RET:
            break;
        default:
            assert(false && "FunctionBuild::CFG - Unexpected Opcode found\n");
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
