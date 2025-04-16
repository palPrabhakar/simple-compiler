#pragma once

#include "program.hpp"
#include <memory>
#include <vector>

namespace sc {
const std::vector<Block *> GetPostOrder(Function *func);
std::unique_ptr<Program> BuildCFG(std::unique_ptr<Program> func);
}; // namespace sc
