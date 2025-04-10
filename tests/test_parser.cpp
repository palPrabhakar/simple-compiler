#include "parser.hpp"
#include "program.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

#define READ_RESULT(x)                                                         \
    std::ifstream testf(x);                                                    \
    std::stringstream testp;                                                   \
    testp << testf.rdbuf();

#define READ_PROGRAM(x)                                                        \
    std::ifstream ifs(x);                                                      \
    std::stringstream output;                                                  \
    auto json_parser = sjp::Parser(ifs);                                       \
    auto data = json_parser.Parse();                                           \
    auto program = sc::ParseProgram(data);

namespace sc {
std::unique_ptr<Program> ParseProgram(sjp::Json);
}

TEST(ParserTest, ParseAdd) {
    READ_PROGRAM("../tests/bril/add.json")

    EXPECT_EQ(program->GetSize(), 1);
    auto &func = program->GetFunction(0);
    func->Dump(output);

    READ_RESULT("../tests/bril/add.bril")

    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseAckermann) {
    READ_PROGRAM("../tests/bril/ackermann.json")

    EXPECT_EQ(program->GetSize(), 2);
    auto &func0 = program->GetFunction(0);
    auto &func1 = program->GetFunction(1);
    func0->Dump(output);
    output << "\n";
    func1->Dump(output);

    READ_RESULT("../tests/bril/ackermann.bril")

    EXPECT_EQ(output.str(), testp.str());
}
