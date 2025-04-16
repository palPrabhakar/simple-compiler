#pragma once

#include "function.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace sc {

using func_ptr = std::unique_ptr<Function>;

class Program {
  public:
    ~Program() = default;
    void AddFunction(std::unique_ptr<Function> func) {
        functions.push_back(std::move(func));
    }
    size_t GetSize() const { return functions.size(); }
    // non-owning pointer
    Function *GetFunction(size_t idx) { return functions[idx].get(); }

    void Dump(std::ostream &out = std::cout) {
        for (auto &f : functions) {
            f->Dump(out);
        }
    }

    // Iterators
    std::vector<func_ptr>::iterator begin() { return functions.begin(); }

    std::vector<func_ptr>::const_iterator begin() const {
        return functions.begin();
    }

    std::vector<func_ptr>::const_iterator cbegin() const {
        return functions.cbegin();
    }

    std::vector<func_ptr>::iterator end() { return functions.end(); }

    std::vector<func_ptr>::const_iterator end() const {
        return functions.end();
    }

    std::vector<func_ptr>::const_iterator cend() const {
        return functions.cend();
    }

  private:
    std::vector<std::unique_ptr<Function>> functions;
};
} // namespace sc
