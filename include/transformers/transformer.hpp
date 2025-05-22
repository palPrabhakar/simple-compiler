#pragma once

#include "program.hpp"
#include <memory>

namespace sc {


template <typename TT>
std::unique_ptr<Program> ApplyTransformation(std::unique_ptr<Program> program) {
    for (auto &f : *program) {
        TT t(f.get());
        t.Transform();
    }
    return program;
}

class Transformer {
  public:
    virtual ~Transformer() = default;
    virtual void Transform() = 0;

  protected:
    Function *func;

    Transformer(Function *f) : func(f) {}
};
} // namespace sc
