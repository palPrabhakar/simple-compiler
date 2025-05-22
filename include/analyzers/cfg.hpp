#pragma once

#include "program.hpp"
#include <memory>
#include <ranges>
#include <vector>

namespace sc {
const std::vector<Block *> GetPostOrder(Function *func);
const std::vector<Block *> GetReversePostOrder(Function *func);
std::unique_ptr<Program> BuildCFG(std::unique_ptr<Program> func);
}; // namespace sc
