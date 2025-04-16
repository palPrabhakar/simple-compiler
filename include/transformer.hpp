#pragma once

#include "program.hpp"
#include <memory>

namespace sc {
class Transformer {
  public:
    virtual ~Transformer() = default;
    virtual std::unique_ptr<Program>
    Transform(std::unique_ptr<Program> program) = 0;

  protected:
    Transformer() = default;
};

class CFTransformer : public Transformer {
  public:
    std::unique_ptr<Program>
    Transform(std::unique_ptr<Program> program) override;
};

class EarlyIRTransformer : public Transformer {
  public:
    EarlyIRTransformer() {}
    std::unique_ptr<Program>
    Transform(std::unique_ptr<Program> program) override {
        for (auto &f : *program) {
            FixIR(f.get());
        }
        return program;
    }

  private:
    void FixIR(Function *func);
    void AddUniqueExitBlock(std::vector<Block *> rb, Function *func);
};

} // namespace sc
