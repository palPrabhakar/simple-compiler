#pragma once

#include "transformer.hpp"
#include <vector>

namespace sc {
class EarlyIRTransformer final : public Transformer {
  public:
    EarlyIRTransformer(Function *f) : Transformer(f) {}
    void Transform() override;

  private:
    std::vector<Block *> rb;
    void CanonicalizeIR();
    void AddUniqueEntryBlock();
    void AddUniqueExitBlock();
    void InsertJmpInstruction(Block* blk, Block* jmp_blk);
    void InsertRetInstruction(Block *blk);
};
} // namespace sc
