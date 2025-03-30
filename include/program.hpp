#pragma once

#include "function.hpp"
#include <memory>
#include <vector>

class Program {
  public:
    void AddFunction(std::unique_ptr<Function> func) {
        functions.push_back(std::move(func));
    }

  private:
    std::vector<std::unique_ptr<Function>> functions;
};
