#include <iostream>
#include <fstream>
#include "parser.hpp"

int main(int argc, char *argv[]) {
    std::cout << "Simple Compiler\n";
    if (argc < 2)
        return 0;
    std::string file_name(argv[1]);
    std::ifstream ifs(file_name);
    auto parser = sjp::Parser(ifs);
    auto data = parser.Parse();
    data.Dump();
    return 0;
}
