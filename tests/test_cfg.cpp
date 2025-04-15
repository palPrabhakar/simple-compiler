#include "block.hpp"
#include "parser.hpp"
#include "program.hpp"
#include <fstream>
#include <gtest/gtest.h>

#define READ_PROGRAM(x)                                                        \
    std::ifstream ifs(x);                                                      \
    auto json_parser = sjp::Parser(ifs);                                       \
    auto data = json_parser.Parse();                                           \
    auto program = sc::ParseProgram(data);

#define BUILD_CFG                                                              \
    for (size_t i = 0; i < program->GetSize(); ++i) {                          \
        auto func = program->GetFunction(i);                                   \
        func->BuildCFG();                                                      \
    }

#define CHECK_SUCC(i, n)                                                       \
    block = func->GetBlock(i);                                                 \
    EXPECT_EQ(block->GetSuccessorSize(), n);

namespace sc {
std::unique_ptr<Program> ParseProgram(sjp::Json);
}

TEST(CFGTest, CFGAdd) {
    READ_PROGRAM("../tests/bril/add.json")
    EXPECT_EQ(program->GetSize(), 1);
    BUILD_CFG
    auto *func = program->GetFunction(0);
    EXPECT_EQ(func->GetBlockSize(), 1);
    sc::Block *block = nullptr;
    CHECK_SUCC(0, 0)
}

TEST(CFGTest, CFGAckermann) {
    READ_PROGRAM("../tests/bril/ackermann.json")
    EXPECT_EQ(program->GetSize(), 2);
    BUILD_CFG
    auto *func = program->GetFunction(0);
    EXPECT_EQ(func->GetBlockSize(), 6);
    sc::Block *block = nullptr;
    CHECK_SUCC(0, 2)
    CHECK_SUCC(1, 1)
    CHECK_SUCC(2, 2)
    CHECK_SUCC(3, 1)
    CHECK_SUCC(4, 1)
    CHECK_SUCC(5, 0)

    // main
    func = program->GetFunction(1);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCC(0, 0)
}

TEST(CFGTest, CFG1dconv) {
    READ_PROGRAM("../tests/bril/1dconv.json")
    EXPECT_EQ(program->GetSize(), 4);
    BUILD_CFG
    // genarray
    auto *func = program->GetFunction(0);
    EXPECT_EQ(func->GetBlockSize(), 8);
    sc::Block *block = nullptr;
    CHECK_SUCC(0, 2)
    CHECK_SUCC(1, 1)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 1)
    CHECK_SUCC(4, 2)
    CHECK_SUCC(5, 1)
    CHECK_SUCC(6, 1)
    CHECK_SUCC(7, 0)

    // convolve
    func = program->GetFunction(1);
    EXPECT_EQ(func->GetBlockSize(), 8);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 2)
    CHECK_SUCC(4, 1)
    CHECK_SUCC(5, 1)
    CHECK_SUCC(6, 1)
    CHECK_SUCC(7, 0)

    // printoutput
    func = program->GetFunction(2);
    EXPECT_EQ(func->GetBlockSize(), 5);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 1)
    CHECK_SUCC(4, 0)

    func = program->GetFunction(3);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCC(0, 0)
}

TEST(CFGTest, CFGRiemann) {
    READ_PROGRAM("../tests/bril/riemann.json")
    EXPECT_EQ(program->GetSize(), 5);
    BUILD_CFG
    sc::Block *block = nullptr;
    sc::Function *func = nullptr;

    // main
    func = program->GetFunction(0);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCC(0, 0)

    // square_function
    func = program->GetFunction(1);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCC(0, 0)

    // left_riemann
    func = program->GetFunction(2);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 0)

    // left_riemann
    func = program->GetFunction(3);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 0)

    // midpoint_riemann
    func = program->GetFunction(4);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 0)
}

TEST(CFGTest, CFGGol) {
    READ_PROGRAM("../tests/bril/gol.json")
    EXPECT_EQ(program->GetSize(), 9);
    BUILD_CFG
    sc::Block *block = nullptr;
    sc::Function *func = nullptr;

    // main
    func = program->GetFunction(0);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCC(0, 0)

    // next_board
    func = program->GetFunction(1);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 0)

    // next_cell
    func = program->GetFunction(2);
    EXPECT_EQ(func->GetBlockSize(), 7);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 2)
    CHECK_SUCC(3, 2)
    CHECK_SUCC(4, 1)
    CHECK_SUCC(5, 1)
    CHECK_SUCC(6, 0)

    // test_neighbors
    func = program->GetFunction(3);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 0)

    // alive
    func = program->GetFunction(4);
    EXPECT_EQ(func->GetBlockSize(), 15);
    CHECK_SUCC(0, 2)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 2)
    CHECK_SUCC(3, 1)
    CHECK_SUCC(4, 1)
    CHECK_SUCC(5, 2)
    CHECK_SUCC(6, 2)
    CHECK_SUCC(7, 1)
    CHECK_SUCC(8, 1)
    CHECK_SUCC(9, 2)
    CHECK_SUCC(10, 2)
    CHECK_SUCC(10, 2)
    CHECK_SUCC(11, 1)
    CHECK_SUCC(12, 2)
    CHECK_SUCC(13, 1)
    CHECK_SUCC(14, 0)

    // rand
    func = program->GetFunction(5);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCC(0, 0)

    // rand_array
    func = program->GetFunction(6);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 0)

    // print_array
    func = program->GetFunction(7);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCC(0, 1)
    CHECK_SUCC(1, 2)
    CHECK_SUCC(2, 1)
    CHECK_SUCC(3, 0)

    // mod
    func = program->GetFunction(8);
    EXPECT_EQ(func->GetBlockSize(), 3);
    CHECK_SUCC(0, 2)
    CHECK_SUCC(1, 1)
    CHECK_SUCC(2, 0)
}
