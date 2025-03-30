#include "parser.hpp"
#include "function.hpp"
#include "program.hpp"

namespace sc {
Program Parse(sjp::Json jprogram) {
    Program program;
    try {
        auto functions = jprogram.Get("functions").value();
        for (size_t i = 0; i < functions.Size(); ++i) {
            auto fname =
                functions.Get(i)->Get("name")->Get<std::string>().value();
            program.AddFunction(std::make_unique<Function>(fname));
        }
    } catch (std::bad_optional_access &err) {
        throw std::runtime_error("Unable to Parse Bril Progam\n");
    }
    return program;
}
} // namespace sc
