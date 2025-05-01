#pragma once

#include "bril_parser.hpp"
#include "cfg.hpp"
#include "program.hpp"
#include "transformer.hpp"
#include <fstream>

#define READ_PROGRAM(x)                                                        \
    std::ifstream ifs(x);                                                      \
    auto parser = sc::BrilParser(ifs);                                         \
    auto program = parser.ParseProgram();

#define BUILD_CFG()                                                            \
    program =                                                                  \
        sc::ApplyTransformation<sc::EarlyIRTransformer>(std::move(program));   \
    program = BuildCFG(std::move(program));

#define DUMP_PROGRAM                                                           \
    std::stringstream output;                                                  \
    for (size_t i = 0; i < program->GetSize(); ++i) {                          \
        auto func = program->GetFunction(i);                                   \
        func->Dump(output);                                                    \
    }
