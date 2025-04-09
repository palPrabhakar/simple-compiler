#include "function.hpp"

namespace sc {
void Function::Dump(std::ostream &out) {
    out<<"@";
    out<<name;
    out<<" {\n";
    for(auto &instr : instructions) {
        out<<"  ";
        instr->Dump(out);
    }
    out<<"}\n";
}
} // namespace sc
