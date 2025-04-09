#include "function.hpp"

namespace sc {
void Function::Dump(std::ostream &out) {
    out << "@" << name << " {\n";
    for (auto &instr : instructions) {
        instr->Dump(out << "  ");
    }
    out << "}\n";
}
} // namespace sc
