#include "cfg.hpp"
#include <memory>
#include <ranges>
#include <stack>

namespace sc {
const std::vector<Block *> GetPostOrder(Function *func) {
    std::vector<Block *> post_order;
    std::vector<bool> visited(func->GetBlockSize(), false);
    std::stack<Block *> st;
    st.push(func->GetBlock(0));
    while (!st.empty()) {
        auto *block = st.top();
        if (!visited[block->GetId()] && block->GetSuccessorSize() > 0) {
            for (size_t i : std::views::iota(0UL, block->GetSuccessorSize())) {
                st.push(block->GetSuccessor(i));
            }
            visited[block->GetId()] = true;
        } else {
            post_order.push_back(block);
            st.pop();
        }
    }
    return post_order;
}

static void BuildCFGImpl(Function *func) {
    auto add_nodes = [func](InstructionBase *instr, size_t idx, size_t start,
                            size_t end) {
        for (size_t i : std::views::iota(start, end)) {
            auto si = static_cast<LabelOperand *>(instr->GetOperand(i))
                          ->GetBlockIdx();
            func->GetBlock(idx)->AddSuccessor(func->GetBlock(si));
            func->GetBlock(si)->AddPredecessor(func->GetBlock(idx));
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
            assert(i == func->GetBlockSize() - 1 &&
                   "Function::BuildCFG - ret instruction found befor last "
                   "block\n");
            break;
        default:
            assert(false && "FunctionBuild::CFG - Unexpted opcode found\n");
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
