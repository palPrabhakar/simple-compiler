#pragma once

#include "program.hpp"
#include <memory>
#include <type_traits>

namespace sc {

class Transformer;

template <typename T>
concept Transformers =
    std::is_base_of_v<Transformer, T> && !std::is_same_v<Transformer, T>;

template <Transformers T>
std::unique_ptr<Program> ApplyTransformation(std::unique_ptr<Program> program) {
    for (auto &f : *program) {
        T t(f.get());
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
