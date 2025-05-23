#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <sstream>

#define READ_RESULT(x)                                                         \
    std::ifstream testf(x);                                                    \
    std::stringstream testp;                                                   \
    testp << testf.rdbuf();

TEST(ParserTest, ParseAdd) {
    READ_PROGRAM("../tests/bril/add.json")
    EXPECT_EQ(program->GetSize(), 1);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/add.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseAckermann) {
    READ_PROGRAM("../tests/bril/ackermann.json")
    EXPECT_EQ(program->GetSize(), 2);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/ackermann.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseBitwiseOps) {
    READ_PROGRAM("../tests/bril/bitwise-ops.json")
    EXPECT_EQ(program->GetSize(), 6);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/bitwise-ops.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseEuclid) {
    READ_PROGRAM("../tests/bril/euclid.json")
    EXPECT_EQ(program->GetSize(), 3);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/euclid.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseFloat) {
    READ_PROGRAM("../tests/bril/float.json")
    EXPECT_EQ(program->GetSize(), 1);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/float.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseRiemann) {
    READ_PROGRAM("../tests/bril/riemann.json")
    EXPECT_EQ(program->GetSize(), 5);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/riemann.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseMem) {
    READ_PROGRAM("../tests/bril/mem.json")
    EXPECT_EQ(program->GetSize(), 1);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/mem.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, Parse1dConv) {
    READ_PROGRAM("../tests/bril/1dconv.json")
    EXPECT_EQ(program->GetSize(), 4);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/1dconv.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseAdler32) {
    READ_PROGRAM("../tests/bril/adler32.json")
    EXPECT_EQ(program->GetSize(), 5);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/adler32.bril")
    EXPECT_EQ(output.str(), testp.str());
}

TEST(ParserTest, ParseGol) {
    READ_PROGRAM("../tests/bril/gol.json")
    EXPECT_EQ(program->GetSize(), 9);
    DUMP_PROGRAM
    READ_RESULT("../tests/bril/gol.bril")
    EXPECT_EQ(output.str(), testp.str());
}
