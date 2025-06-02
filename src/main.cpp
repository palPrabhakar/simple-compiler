#include "analyzers/cfg.hpp"
#include "bril_parser.hpp"
#include "program.hpp"
#include "transformers/transformer.hpp"
#include "transformers/early_ir_transformer.hpp"
#include "transformers/cf_transformer.hpp"
#include "transformers/ssa_transformer.hpp"
#include "transformers/dvn_transformer.hpp"

#include <fstream>
#include <iostream>

namespace sc {
std::unique_ptr<Program> ParseProgram(sjp::Json);
}

int main(int argc, char *argv[]) {
    std::unique_ptr<sc::Program> program;
    if (argc < 2) {
        program = sc::BrilParser::ParseProgram(std::cin);
    } else {
        std::ifstream ifs(argv[1]);
        program = sc::BrilParser::ParseProgram(ifs);
    }

    program =
        sc::ApplyTransformation<sc::EarlyIRTransformer>(std::move(program));
    program = sc::BuildCFG(std::move(program));
    program = sc::ApplyTransformation<sc::CFTransformer>(std::move(program));
    program = sc::ApplyTransformation<sc::SSATransformer>(std::move(program));
    // program = sc::ApplyTransformation<sc::DVNTransformer>(std::move(program));
    program->Dump();


    return 0;
}
