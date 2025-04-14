#include "function.hpp"
#include "instruction.hpp"
#include "operand.hpp"

    namespace sc {
    void Function::Dump(std::ostream & out) {
        out << "@" << name;
        if (args.size()) {
            out << "(";
            out << args[0]->GetName() << ": " << args[0]->GetStrType();
            for (size_t i = 1; i < args.size(); ++i) {
                out << ", " << args[i]->GetName() << ": "
                    << args[i]->GetStrType();
            }
            out << ")";
        }
        if (ret_type != DataType::VOID) {
            out << ": " << GetStrRetType();
        }
        out << " {\n";
        for (auto &block : blocks) {
            for (auto &instr : *block) {
                if (instr->GetOpCode() == OpCode::LABEL) {
                    instr->Dump(out);
                } else {
                    instr->Dump(out << "  ");
                }
            }
        }
        out << "}\n";
    }

    std::string Function::GetStrRetType() const {
        return ret_type != DataType::VOID ? GetStrDataType(ret_type)
                                          : std::string();
    }

    std::string PtrFunction::GetStrRetType() const {
        return GetPtrType(ptr_chain);
    }
} // namespace sc
