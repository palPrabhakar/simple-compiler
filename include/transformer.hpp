#pragma once

#include "cfg.hpp"
#include "program.hpp"
#include <cassert>
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

// class CFTransformer : public Transformer {
//   public:
//     std::unique_ptr<Program> Transform(std::unique_ptr<Program> program) {
//         for (auto &f : *program) {
//             traverse_order = GetPostOrder(f.get());
//             if (traverse_order.size() != f->GetBlockSize()) {
//                 std::cout << "Calling RemoveUnreachableCFNode\n";
//                 RemoveUnreachableCFNode(f.get());
//                 traverse_order = GetPostOrder(f.get());
//             }
//             while (Clean(f.get())) {
//                 traverse_order = GetPostOrder(f.get());
//             }
//         }
//         return program;
//     }

//   private:
//     void RemoveUnreachableCFNode(Function *func);
//     bool Clean(Function *func);
//     void ReplaceBrWithJmp(Block *block);
//     std::vector<Block *> traverse_order;
// };

} // namespace sc
