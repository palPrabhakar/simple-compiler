#include "function.hpp"
#include "instructions.hpp"
#include "operands.hpp"

namespace sc {
void Function::Dump(std::ostream &out) {
    out << "@" << name;
    if (args.size()) {
        out << "(";
        out << args[0]->GetName() << ": " << GetStrDataType(args[0]->GetType());
        for (size_t i = 1; i < args.size(); ++i) {
            out << ", " << args[i]->GetName() << ": "
                << GetStrDataType(args[i]->GetType());
        }
        out << ")";
    }
    if (ret_type != DataType::VOID) {
        out << ": " << GetStrDataType(ret_type);
    }
    out << " {\n";
    for (auto &instr : instructions) {
        if (instr->GetOpCode() == OpCode::LABEL) {
            instr->Dump(out);
        } else {
            instr->Dump(out << "  ");
        }
    }
    out << "}\n";
}
} // namespace sc
