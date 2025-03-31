#include "parser.hpp"
#include "program.hpp"
#include <fstream>
#include <iostream>

namespace sc {
std::unique_ptr<Program> ParseProgram(sjp::Json);
}

int main(int argc, char *argv[]) {
    std::cout << "Simple Compiler\n";
    if (argc < 2)
        return 0;
    std::ifstream ifs(argv[1]);
    auto json_parser = sjp::Parser(ifs);
    auto data = json_parser.Parse();
    auto program = sc::ParseProgram(data);
    return 0;
}
