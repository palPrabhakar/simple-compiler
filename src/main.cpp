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

    auto ir_transformer = sc::EarlyIRTransformer();
    program = ir_transformer.Transform(std::move(program));
    program = sc::BuildCFG(std::move(program));
    auto cf_transformer = sc::CFTransformer();
    program = cf_transformer.Transform(std::move(program));
    program->Dump();

    // for (auto i : std::views::iota(0ul, program->GetSize())) {
    //     std::cout<<"\n=============================\n";
    //     auto *func = program->GetFunction(i);
    //     func->Dump();
    //     std::cout<<"\n";
    //     func->DumpCFG();
    //     std::cout<<"\n";
    //     auto dom = sc::DominatorAnalyzer(func);
    //     dom.ComputeDominance();
    //     dom.DumpDominators();
    //     std::cout<<"\n";
    //     dom.ComputeImmediateDominators();
    //     dom.DumpImmediateDominators();
    //     std::cout<<"\n";
    //     dom.ComputeDominanceFrontier();
    //     dom.DumpDominanceFrontier();
    //     std::cout<<"=============================\n";
    // }

    return 0;
}
