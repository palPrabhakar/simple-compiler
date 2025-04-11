#pragma once

#include "function.hpp"
#include <memory>
#include <vector>

namespace sc {
class Program {
  public:
    ~Program() = default;
    void AddFunction(std::unique_ptr<Function> func) {
        functions.push_back(std::move(func));
    }
    size_t GetSize() const { return functions.size(); }
    // non-owning pointer
    Function *GetFunction(size_t idx) { return functions[idx].get(); }

  private:
    std::vector<std::unique_ptr<Function>> functions;
};
} // namespace sc
