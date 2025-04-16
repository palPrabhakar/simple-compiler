#include "block.hpp"
#include "cfg.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "transformer.hpp"
#include <fstream>
#include <gtest/gtest.h>

#define READ_PROGRAM(x)                                                        \
    std::ifstream ifs(x);                                                      \
    auto json_parser = sjp::Parser(ifs);                                       \
    auto data = json_parser.Parse();                                           \
    auto program = sc::ParseProgram(data);

#define BUILD_CFG                                                              \
    sc::EarlyIRTransformer ir_transformer;                                     \
    program = ir_transformer.Transform(std::move(program));                    \
    program = BuildCFG(std::move(program));

#define CHECKER0(i)                                                            \
    block = func->GetBlock(i);                                                 \
    EXPECT_EQ(block->GetSuccessorSize(), 0);

#define CHECKER1(i, a)                                                         \
    block = func->GetBlock(i);                                                 \
    EXPECT_EQ(block->GetSuccessor(0), func->GetBlock(a));

#define CHECKER2(i, a, b)                                                      \
    block = func->GetBlock(i);                                                 \
    EXPECT_EQ(block->GetSuccessor(0), func->GetBlock(a));                      \
    EXPECT_EQ(block->GetSuccessor(1), func->GetBlock(b));

#define GET_CHECKER(_1, _2, _3, CHECKER, ...) CHECKER
#define CHECK_SUCCESSOR(...)                                                   \
    GET_CHECKER(__VA_ARGS__, CHECKER2, CHECKER1, CHECKER0)(__VA_ARGS__)

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
    CHECK_SUCCESSOR(0)
}

TEST(CFGTest, CFGAckermann) {
    READ_PROGRAM("../tests/bril/ackermann.json")
    EXPECT_EQ(program->GetSize(), 2);
    BUILD_CFG
    auto *func = program->GetFunction(0);
    EXPECT_EQ(func->GetBlockSize(), 6);
    sc::Block *block = nullptr;
    CHECK_SUCCESSOR(0, 1, 2)
    CHECK_SUCCESSOR(1, 5)
    CHECK_SUCCESSOR(2, 3, 4)
    CHECK_SUCCESSOR(3, 5)
    CHECK_SUCCESSOR(4, 5)
    CHECK_SUCCESSOR(5)

    // main
    func = program->GetFunction(1);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCCESSOR(0)
}

TEST(CFGTest, CFG1dconv) {
    READ_PROGRAM("../tests/bril/1dconv.json")
    EXPECT_EQ(program->GetSize(), 4);
    BUILD_CFG
    // genarray
    auto *func = program->GetFunction(0);
    EXPECT_EQ(func->GetBlockSize(), 8);
    sc::Block *block = nullptr;
    CHECK_SUCCESSOR(0, 1, 2)
    CHECK_SUCCESSOR(1, 3)
    CHECK_SUCCESSOR(2, 3)
    CHECK_SUCCESSOR(3, 4)
    CHECK_SUCCESSOR(4, 5, 7)
    CHECK_SUCCESSOR(5, 6)
    CHECK_SUCCESSOR(6, 4)
    CHECK_SUCCESSOR(7)

    // convolve
    func = program->GetFunction(1);
    EXPECT_EQ(func->GetBlockSize(), 8);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 2, 7)
    CHECK_SUCCESSOR(2, 3)
    CHECK_SUCCESSOR(3, 4, 6)
    CHECK_SUCCESSOR(4, 5)
    CHECK_SUCCESSOR(5, 3)
    CHECK_SUCCESSOR(6, 1)
    CHECK_SUCCESSOR(7)

    // printoutput
    func = program->GetFunction(2);
    EXPECT_EQ(func->GetBlockSize(), 5);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 2, 4)
    CHECK_SUCCESSOR(2, 3)
    CHECK_SUCCESSOR(3, 1)
    CHECK_SUCCESSOR(4)

    func = program->GetFunction(3);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCCESSOR(0)
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
    CHECK_SUCCESSOR(0)

    // square_function
    func = program->GetFunction(1);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCCESSOR(0)

    // left_riemann
    func = program->GetFunction(2);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 3, 2)
    CHECK_SUCCESSOR(2, 1)
    CHECK_SUCCESSOR(3)

    // left_riemann
    func = program->GetFunction(3);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 3, 2)
    CHECK_SUCCESSOR(2, 1)
    CHECK_SUCCESSOR(3)

    // midpoint_riemann
    func = program->GetFunction(4);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 3, 2)
    CHECK_SUCCESSOR(2, 1)
    CHECK_SUCCESSOR(3)
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
    CHECK_SUCCESSOR(0)

    // next_board
    func = program->GetFunction(1);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 3, 2)
    CHECK_SUCCESSOR(2, 1)
    CHECK_SUCCESSOR(3)

    // next_cell
    func = program->GetFunction(2);
    EXPECT_EQ(func->GetBlockSize(), 7);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 5, 2)
    CHECK_SUCCESSOR(2, 4, 5)
    CHECK_SUCCESSOR(3, 4, 5)
    CHECK_SUCCESSOR(4, 6)
    CHECK_SUCCESSOR(5, 6)
    CHECK_SUCCESSOR(6)

    // test_neighbors
    func = program->GetFunction(3);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 3, 2)
    CHECK_SUCCESSOR(2, 1)
    CHECK_SUCCESSOR(3)

    // alive
    func = program->GetFunction(4);
    EXPECT_EQ(func->GetBlockSize(), 15);
    CHECK_SUCCESSOR(0, 5, 1)
    CHECK_SUCCESSOR(1, 2, 3)
    CHECK_SUCCESSOR(2, 5, 4)
    CHECK_SUCCESSOR(3, 2)
    CHECK_SUCCESSOR(4, 5)
    CHECK_SUCCESSOR(5, 6, 7)
    CHECK_SUCCESSOR(6, 9, 8)
    CHECK_SUCCESSOR(7, 6)
    CHECK_SUCCESSOR(8, 9)
    CHECK_SUCCESSOR(9, 14, 10)
    CHECK_SUCCESSOR(10, 12, 11)
    CHECK_SUCCESSOR(11, 12)
    CHECK_SUCCESSOR(12, 14, 13)
    CHECK_SUCCESSOR(13, 14)
    CHECK_SUCCESSOR(14)

    // rand
    func = program->GetFunction(5);
    EXPECT_EQ(func->GetBlockSize(), 1);
    CHECK_SUCCESSOR(0)

    // rand_array
    func = program->GetFunction(6);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 3, 2)
    CHECK_SUCCESSOR(2, 1)
    CHECK_SUCCESSOR(3)

    // print_array
    func = program->GetFunction(7);
    EXPECT_EQ(func->GetBlockSize(), 4);
    CHECK_SUCCESSOR(0, 1)
    CHECK_SUCCESSOR(1, 3, 2)
    CHECK_SUCCESSOR(2, 1)
    CHECK_SUCCESSOR(3)

    // mod
    func = program->GetFunction(8);
    EXPECT_EQ(func->GetBlockSize(), 3);
    CHECK_SUCCESSOR(0, 2, 1)
    CHECK_SUCCESSOR(1, 2)
    CHECK_SUCCESSOR(2)
}
