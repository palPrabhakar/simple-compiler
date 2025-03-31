#pragma once

#include "function.hpp"
#include <memory>
#include <vector>

namespace sc {
class Program {
  public:
    void AddFunction(std::unique_ptr<Function> func) {
        functions.push_back(std::move(func));
    }

    ~Program() = default;

    size_t GetSize() const { return functions.size(); }

    std::unique_ptr<Function>& GetFunction(size_t idx) {
        return functions[idx];
    }

  private:
    std::vector<std::unique_ptr<Function>> functions;
};
} // namespace sc
