#include "analyzer.hpp"
#include "bril_parser.hpp"
#include "cfg.hpp"
#include "program.hpp"
#include "transformer.hpp"
#include <fstream>
#include <iostream>
#include <ranges>

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

    program =
        sc::ApplyTransformation<sc::EarlyIRTransformer>(std::move(program));
    program = sc::BuildCFG(std::move(program));
    program = sc::ApplyTransformation<sc::CFTransformer>(std::move(program));
    // program->Dump();

    program = sc::ApplyTransformation<sc::SSATransformer>(std::move(program));
    program->Dump();

    // for (auto i : std::views::iota(0ul, program->GetSize())) {
    //     std::cout<<"\n=============================\n";
    //     auto *func = program->GetFunction(i);
    //     auto global_analyzer = sc::GlobalsAnalyzer(func);
    //     global_analyzer.FindGlobalNames();
    //     global_analyzer.DumpGlobals();
    //     std::cout<<"\n\n";
    //     global_analyzer.DumpBlocks();
    //     std::cout<<"=============================\n";
    // }

    return 0;
}
