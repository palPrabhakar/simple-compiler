#include "function.hpp"
#include "instructions.hpp"

namespace sc {
void Function::Dump(std::ostream &out) {
    out << "@" << name << " {\n";
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
