#include "parser.hpp"
#include "program.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace sc {
std::unique_ptr<Program> ParseProgram(sjp::Json);
}

TEST(ParserTest, ParseAdd) {
    std::ifstream ifs("../tests/bril/add.json");
    auto json_parser = sjp::Parser(ifs);
    auto data = json_parser.Parse();
    auto program = sc::ParseProgram(data);
    EXPECT_EQ(program->GetSize(),  1);
    auto &func = program->GetFunction(0);
    EXPECT_EQ(func->GetArgsSize(),  0);
    EXPECT_EQ(func->GetInstructionSize(), 4);
    EXPECT_EQ(func->GetInstruction(0)->GetOpCode(), sc::OpCode::CONST);
    EXPECT_EQ(func->GetInstruction(1)->GetOpCode(), sc::OpCode::CONST);
    EXPECT_EQ(func->GetInstruction(2)->GetOpCode(), sc::OpCode::ADD);
    EXPECT_EQ(func->GetInstruction(3)->GetOpCode(), sc::OpCode::PRINT);
}
