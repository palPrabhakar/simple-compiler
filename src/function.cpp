#include "function.hpp"

namespace sc {
void Function::Dump(std::ostream &out) {
    out<<"@";
    out<<name;
    out<<" {\n";
    out<<"}\n";
}
} // namespace sc
