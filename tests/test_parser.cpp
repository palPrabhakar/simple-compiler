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
    EXPECT_EQ(func->GetName(), "main");
    EXPECT_EQ(func->GetArgsSize(),  0);
    EXPECT_EQ(func->GetInstructionSize(), 4);
    EXPECT_EQ(func->GetInstruction(0)->GetOpCode(), sc::OpCode::CONST);
    EXPECT_EQ(func->GetInstruction(1)->GetOpCode(), sc::OpCode::CONST);
    EXPECT_EQ(func->GetInstruction(2)->GetOpCode(), sc::OpCode::ADD);
    EXPECT_EQ(func->GetInstruction(3)->GetOpCode(), sc::OpCode::PRINT);
}

TEST(ParserTest, ParseAckermann) {
    std::ifstream ifs("../tests/bril/ackermann.json");
    auto json_parser = sjp::Parser(ifs);
    auto data = json_parser.Parse();
    auto program = sc::ParseProgram(data);
    EXPECT_EQ(program->GetSize(),  2);
    auto &func0 = program->GetFunction(0);
    EXPECT_EQ(func0->GetName(), "ack");
    EXPECT_EQ(func0->GetArgsSize(), 2);
    EXPECT_EQ(func0->GetInstructionSize(), 20);
    EXPECT_EQ(func0->GetInstruction(0)->GetOpCode(), sc::OpCode::CONST);
    EXPECT_EQ(func0->GetInstruction(1)->GetOpCode(), sc::OpCode::CONST);
    EXPECT_EQ(func0->GetInstruction(2)->GetOpCode(), sc::OpCode::EQ);
    EXPECT_EQ(func0->GetInstruction(3)->GetOpCode(), sc::OpCode::BR);
    EXPECT_EQ(func0->GetInstruction(4)->GetOpCode(), sc::OpCode::LABEL);
    EXPECT_EQ(func0->GetInstruction(5)->GetOpCode(), sc::OpCode::ADD);
    EXPECT_EQ(func0->GetInstruction(6)->GetOpCode(), sc::OpCode::RET);
    EXPECT_EQ(func0->GetInstruction(7)->GetOpCode(), sc::OpCode::LABEL);
    EXPECT_EQ(func0->GetInstruction(8)->GetOpCode(), sc::OpCode::EQ);
    EXPECT_EQ(func0->GetInstruction(9)->GetOpCode(), sc::OpCode::BR);
    EXPECT_EQ(func0->GetInstruction(10)->GetOpCode(), sc::OpCode::LABEL);
    EXPECT_EQ(func0->GetInstruction(11)->GetOpCode(), sc::OpCode::SUB);
    EXPECT_EQ(func0->GetInstruction(12)->GetOpCode(), sc::OpCode::CALL);
    EXPECT_EQ(func0->GetInstruction(13)->GetOpCode(), sc::OpCode::RET);
    EXPECT_EQ(func0->GetInstruction(14)->GetOpCode(), sc::OpCode::LABEL);
    EXPECT_EQ(func0->GetInstruction(15)->GetOpCode(), sc::OpCode::SUB);
    EXPECT_EQ(func0->GetInstruction(16)->GetOpCode(), sc::OpCode::SUB);
    EXPECT_EQ(func0->GetInstruction(17)->GetOpCode(), sc::OpCode::CALL);
    EXPECT_EQ(func0->GetInstruction(18)->GetOpCode(), sc::OpCode::CALL);
    EXPECT_EQ(func0->GetInstruction(19)->GetOpCode(), sc::OpCode::RET);
    auto &func1 = program->GetFunction(1);
    EXPECT_EQ(func1->GetName(), "main");
    EXPECT_EQ(func1->GetArgsSize(), 2);
    EXPECT_EQ(func1->GetInstructionSize(), 2);
    EXPECT_EQ(func1->GetInstruction(0)->GetOpCode(), sc::OpCode::CALL);
    EXPECT_EQ(func1->GetInstruction(1)->GetOpCode(), sc::OpCode::PRINT);
}
