#pragma once

#include "transformer.hpp"
#include <vector>
#include <ranges>

namespace sc {
class CFTransformer final : public Transformer {
  public:
    CFTransformer(Function *f) : Transformer(f) {}

    void Transform() override;

  private:
    std::vector<Block *> traverse_order;

    void RemoveUnreachableCFNode();
    bool Clean();
    void ReplaceBrWithJmp(Block *block);
    void RemoveEmptyBlock(Block *block);
    void CombineBlocks(Block *block);
    void HoistBranch(Block *block);
};
} // namespace sc
