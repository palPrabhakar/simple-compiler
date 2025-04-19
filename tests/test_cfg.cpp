#include "block.hpp"
#include "bril_parser.hpp"
#include "program.hpp"
#include "transformer.hpp"
#include <fstream>
#include <gtest/gtest.h>

#define READ_PROGRAM(x)                                                        \
    std::ifstream ifs(x);                                                      \
    auto parser = sc::BrilParser(ifs);                                         \
    auto program = parser.ParseProgram();

#define BUILD_CFG()                                                            \
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

#define CHECK_CFG()                                                            \
    EXPECT_EQ(program->GetSize(), successors.size());                          \
    EXPECT_EQ(successors.size(), predecessors.size());                         \
    for (auto f : std::views::iota(0UL, program->GetSize())) {                 \
        auto *func = program->GetFunction(f);                                  \
        EXPECT_EQ(func->GetBlockSize(), successors[f].size());                 \
        for (auto b : std::views::iota(0UL, func->GetBlockSize())) {           \
            auto *block = func->GetBlock(b);                                   \
            EXPECT_EQ(successors[f][b].size(), block->GetSuccessorSize());     \
            for (auto s : std::views::iota(0UL, block->GetSuccessorSize())) {  \
                EXPECT_EQ(block->GetSuccessor(s),                              \
                          func->GetBlock(successors[f][b][s]));                \
            }                                                                  \
            EXPECT_EQ(predecessors[f][b].size(), block->GetPredecessorSize()); \
            for (auto p :                                                      \
                 std::views::iota(0UL, block->GetPredecessorSize())) {         \
                EXPECT_EQ(block->GetPredecessor(p),                            \
                          func->GetBlock(predecessors[f][b][p]));              \
            }                                                                  \
        }                                                                      \
        auto po = sc::GetPostOrder(func);                                      \
        EXPECT_EQ(po.size(), postorder[f].size());                             \
        for (auto o : std::views::iota(0UL, po.size())) {                      \
            EXPECT_EQ(po[o], func->GetBlock(postorder[f][o]));                 \
        }                                                                      \
    }

namespace sc {
std::unique_ptr<Program> ParseProgram(sjp::Json);
}

TEST(CFGTest, CFGAdd) {
    // clang-format off
    std::vector<std::vector<std::vector<size_t>>> successors = {
        {{}} // main
    };
    std::vector<std::vector<std::vector<size_t>>> predecessors = {
        {{}}
    };
    std::vector<std::vector<size_t>> postorder = {
        {0}
    };
    // clang-format on

    READ_PROGRAM("../tests/bril/add.json")
    BUILD_CFG()
    CHECK_CFG()
}

TEST(CFGTest, CFGAckermann) {
    // clang-format off
    std::vector<std::vector<std::vector<size_t>>> successors = {
        {{1, 2}, {5}, {3, 4}, {5}, {5}, {}},    // ack
        {{}}                                    // main
    };
    std::vector<std::vector<std::vector<size_t>>> predecessors = {
        {{}, {0}, {0}, {2}, {2}, {1, 3, 4}},
        {{}}
    };
    std::vector<std::vector<size_t>> postorder = {
        {5, 1, 3, 4, 2, 0},
        {0}
    };
    // clang-format on

    READ_PROGRAM("../tests/bril/ackermann.json")
    BUILD_CFG()
    CHECK_CFG()
}

TEST(CFGTest, CFG1dconv) {
    // clang-format off
    std::vector<std::vector<std::vector<size_t>>> successors = {
        {{1, 2}, {3}, {3}, {4}, {5, 7}, {6}, {4}, {}},  // genarry
        {{1}, {2, 7}, {3}, {4, 6}, {5}, {3}, {1}, {}},  // convolve
        {{1}, {2, 4}, {3}, {1}, {}},                    // printoutput
        {{}}                                            // main
    };
    std::vector<std::vector<std::vector<size_t>>> predecessors = {
        {{}, {0}, {0}, {1, 2}, {3, 6}, {4}, {5}, {4}},
        {{}, {0, 6}, {1}, {2, 5}, {3}, {4}, {3}, {1}},
        {{}, {0, 3}, {1}, {2}, {1}},
        {{}}
    };
    std::vector<std::vector<size_t>> postorder = {
        {6, 5, 7, 4, 3, 1, 2, 0},
        {5, 4, 6, 3, 2, 7, 1, 0},
        {3, 2, 4, 1, 0},
        {0}
    };
    // clang-format on

    READ_PROGRAM("../tests/bril/1dconv.json")
    BUILD_CFG()
    CHECK_CFG()
}

TEST(CFGTest, CFGRiemann) {
    // clang-format off
    std::vector<std::vector<std::vector<size_t>>> successors = {
        {{}}, // main
        {{}}, // square_function
        {{1}, {3, 2}, {1}, {}}, // left_riemann
        {{1}, {3, 2}, {1}, {}}, // right_riemann
        {{1}, {3, 2}, {1}, {}}  // midpoint_riemann
    };
    std::vector<std::vector<std::vector<size_t>>> predecessors = {
        {{}}, // main
        {{}}, // square_function
        {{}, {0, 2}, {1}, {1}}, // left_riemann
        {{}, {0, 2}, {1}, {1}}, // right_riemann
        {{}, {0, 2}, {1}, {1}}  // midpoint_riemann
    };
    std::vector<std::vector<size_t>> postorder = {
        {0},
        {0},
        {3, 2, 1, 0},
        {3, 2, 1, 0},
        {3, 2, 1, 0}
    };
    // clang-format on

    READ_PROGRAM("../tests/bril/riemann.json")
    BUILD_CFG()
    CHECK_CFG()
}

TEST(CFGTest, CFGGol) {
    // clang-format off
    std::vector<std::vector<std::vector<size_t>>> successors = {
        {{}},                                               // main
        {{1}, {3, 2}, {1}, {}},                             // next_board
        {{1, 3}, {5, 2}, {4, 5}, {4, 5}, {6}, {6}, {}},     // next_cell
        {{1}, {3, 2}, {1}, {}},                             // test_neighbors
        {
            {5, 1}, {2, 3}, {5, 4}, {2}, {5}, {6, 7}, {9, 8},
            {6}, {9}, {14, 10}, {12, 11}, {12}, {14, 13}, {14}, {},
        },                                                  // alive
        {{}},                                               // rand
        {{1}, {3, 2}, {1}, {}},                             // rand_array
        {{1}, {3, 2}, {1}, {}},                             // print_array
        {{2, 1}, {2}, {}}                                   // mod
    };
    std::vector<std::vector<std::vector<size_t>>> predecessors = {
        {{}},                                               // main
        {{}, {0, 2}, {1}, {1}},                             // next_board
        {{}, {0}, {1}, {0}, {2, 3}, {1, 2, 3}, {4, 5}},     // next_cell
        {{}, {0, 2}, {1}, {1}},                             // test_neighbors
        {
            {}, {0}, {1, 3}, {1}, {2}, {0, 2, 4}, {5, 7},
            {5}, {6}, {6, 8}, {9}, {10}, {10, 11}, {12}, {9, 12, 13}
        },                                                  // alive
        {{}},                                               // rand
        {{}, {0, 2}, {1}, {1}},                             // rand_array
        {{}, {0, 2}, {1}, {1}},                             // print_array
        {{}, {0}, {0, 1}}                                   // mod
    };
    std::vector<std::vector<size_t>> postorder = {
        {0},                                                // main
        {3, 2, 1, 0},                                       // next_board
        {6, 5, 4, 2, 1, 3, 0},                              // next_cell
        {3, 2, 1, 0},                                       // test_neighbors
        {14, 13, 12, 11, 10, 9, 8, 6, 7, 5, 4, 2, 3, 1, 0}, // alive
        {0},                                                // rand
        {3, 2, 1, 0},                                       // rand_array
        {3, 2, 1, 0},                                       // print_array
        {2, 1, 0}                                           // mod
    };

    // clang-format on
    READ_PROGRAM("../tests/bril/gol.json")
    EXPECT_EQ(program->GetSize(), 9);
    BUILD_CFG()
    CHECK_CFG()
}
