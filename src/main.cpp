#include "cfg.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "transformer.hpp"
#include <fstream>
#include <iostream>

namespace sc {
std::unique_ptr<Program> ParseProgram(sjp::Json);
}

int main(int argc, char *argv[]) {
    std::unique_ptr<sc::Program> program;
    if (argc < 2) {
        auto json_parser = sjp::Parser(std::cin);
        auto data = json_parser.Parse();
        program = sc::ParseProgram(data);
    } else {
        std::ifstream ifs(argv[1]);
        auto json_parser = sjp::Parser(ifs);
        auto data = json_parser.Parse();
        program = sc::ParseProgram(data);
    }
    auto ir_transformer = sc::EarlyIRTransformer();
    program = ir_transformer.Transform(std::move(program));
    program = sc::BuildCFG(std::move(program));
    auto cf_transformer = sc::CFTransformer();
    program = cf_transformer.Transform(std::move(program));
    program->Dump();
    return 0;
}
