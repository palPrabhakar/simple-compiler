#include "function.hpp"
#include "block.hpp"
#include "operand.hpp"

namespace sc {
void Function::Dump(std::ostream &out) const {
    out << "@" << name;
    if (args.size()) {
        out << "(";
        out << args[0]->GetName() << ": " << args[0]->GetStrType();
        for (size_t i = 1; i < args.size(); ++i) {
            out << ", " << args[i]->GetName() << ": " << args[i]->GetStrType();
        }
        out << ")";
    }
    if (ret_type != DataType::VOID) {
        out << ": " << GetStrRetType();
    }
    out << " {\n";
    for (auto &block : blocks) {
        out << "." << block->GetName() << ":\n";
        block->Dump(out, "  ");
    }
    out << "}\n";
}

std::string Function::GetStrRetType() const {
    return ret_type != DataType::VOID ? GetStrDataType(ret_type)
                                      : std::string();
}

void Function::DumpBlocks(std::ostream &out) const {
    out << "function: " << name << "\n";
    for (auto &block : blocks) {
        out << "  " << block->GetName() << "\n";
    }
    out << "\n";
}

void Function::DumpCFG(std::ostream &out) const {
    out << "CFG function: " << name << "\n";
    for (auto &block : blocks) {
        out << "  " << block->GetName() << ":\n";
        out << "    successors: ";
        for (auto succ : block->GetSuccessors()) {
            out << succ->GetName() << " ";
        }
        out << "\n    predecessors: ";
        for (auto pred : block->GetPredecessors()) {
            out << pred->GetName() << " ";
        }
        out << "\n";
    }
    out << "\n";
}

void Function::DumpDefUseLinks(std::ostream &out) const {
    for (auto &block : blocks) {
        for (auto *instr : block->GetInstructions()) {
            if (instr->HasDest()) {
                out << "Def: ";
                instr->Dump(out);
                auto *dest = instr->GetDest();
                for (auto *use : dest->GetUses()) {
                    use->Dump(out << "\tUse: ");
                }
                out << "\n";
            }
        }
    }
}

std::string PtrFunction::GetStrRetType() const { return GetPtrType(ptr_chain); }
} // namespace sc
