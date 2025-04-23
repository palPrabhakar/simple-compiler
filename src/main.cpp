#include "analyzer.hpp"
#include "bril_parser.hpp"
#include "cfg.hpp"
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
        auto parser = sc::BrilParser(std::cin);
        program = parser.ParseProgram();
    } else {
        std::ifstream ifs(argv[1]);
        auto parser = sc::BrilParser(ifs);
        program = parser.ParseProgram();
    }
    auto ir_transformer = sc::EarlyIRTransformer();
    program = ir_transformer.Transform(std::move(program));
    program = sc::BuildCFG(std::move(program));
    auto cf_transformer = sc::CFTransformer();
    program = cf_transformer.Transform(std::move(program));
    program->Dump();
    auto dom = sc::DominatorAnalyzer(program->GetFunction(0));
    dom.ComputeDominance();
    dom.DumpDominators();
    return 0;
}
