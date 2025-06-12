#pragma once

#include "instruction.hpp"
#include "instruction_visitor.hpp"
#include "transformer.hpp"
#include <memory>
#include <type_traits>
#include <unordered_map>

namespace sc {

// Lattice value type
enum class LVT { INDETERMINABLE, CONSTANT, UNKNOWN };

class ConstantPropagator;

class SSCPTransformer final : public Transformer {
  public:
    SSCPTransformer(Function *f)
        : Transformer(f),
          propagator(std::make_unique<ConstantPropagator>(this)) {}

    void Transform() override {
        Initialize();
        Propagate();
        Simplify();
    }

  private:
    std::unordered_map<OperandBase *, LVT> values;
    std::unordered_map<OperandBase *, OperandBase *> constants;
    std::vector<OperandBase *> worklist;
    std::unique_ptr<ConstantPropagator> propagator;
    std::vector<size_t> remove;

    friend ConstantPropagator;

    void Initialize();
    void Propagate();
    void Simplify();
    void SimplifyCFG();
};

class ConstantPropagator : public InstructionVisitor {
  public:
    ConstantPropagator(SSCPTransformer *transformer) : sscp(transformer) {}
    // Arithmetic
    void VisitAddInstruction(AddInstruction *instr) override {
        ProcessBinaryInstruction<std::plus, IntOperand>(instr);
    }

    void VisitMulInstruction(MulInstruction *instr) override {
        ProcessMulInstruction<IntOperand>(instr);
    }

    void VisitSubInstruction(SubInstruction *instr) override {
        ProcessBinaryInstruction<std::minus, IntOperand>(instr);
    }

    void VisitDivInstruction(DivInstruction *instr) override {
        ProcessDivInstruction<IntOperand>(instr);
    }

    // Comparison
    void VisitEqInstruction(EqInstruction *instr) override {
        ProcessBinaryInstruction<std::equal_to, IntOperand>(instr);
    }

    void VisitLtInstruction(LtInstruction *instr) override {
        ProcessBinaryInstruction<std::less, IntOperand>(instr);
    }

    void VisitGtInstruction(GtInstruction *instr) override {
        ProcessBinaryInstruction<std::greater, IntOperand>(instr);
    }

    void VisitLeInstruction(LeInstruction *instr) override {
        ProcessBinaryInstruction<std::less_equal, IntOperand>(instr);
    }

    void VisitGeInstruction(GeInstruction *instr) override {
        ProcessBinaryInstruction<std::greater_equal, IntOperand>(instr);
    }

    // Logic
    void VisitAndInstruction(AndInstruction *instr) override;
    void VisitOrInstruction(OrInstruction *instr) override;
    void VisitNotInstruction(NotInstruction *instr) override;

    // SSA
    // void VisitSetInstruction(SetInstruction *instr) override;
    void VisitGetInstruction(GetInstruction *instr) override;

    // Floating Arithmetic
    void VisitFAddInstruction(FAddInstruction *instr) override {
        ProcessBinaryInstruction<std::plus, FloatOperand>(instr);
    }

    void VisitFMulInstruction(FMulInstruction *instr) override {
        ProcessMulInstruction<FloatOperand>(instr);
    }

    void VisitFSubInstruction(FSubInstruction *instr) override {
        ProcessBinaryInstruction<std::minus, FloatOperand>(instr);
    }

    void VisitFDivInstruction(FDivInstruction *instr) override {
        ProcessDivInstruction<FloatOperand>(instr);
    }

    // Floating Comparison
    void VisitFEqInstruction(FEqInstruction *instr) override {
        ProcessBinaryInstruction<std::equal_to, FloatOperand>(instr);
    }

    void VisitFLtInstruction(FLtInstruction *instr) override {
        ProcessBinaryInstruction<std::less, FloatOperand>(instr);
    }

    void VisitFGtInstruction(FGtInstruction *instr) override {
        ProcessBinaryInstruction<std::greater, FloatOperand>(instr);
    }

    void VisitFLeInstruction(FLeInstruction *instr) override {
        ProcessBinaryInstruction<std::less_equal, FloatOperand>(instr);
    }

    void VisitFGeInstruction(FGeInstruction *instr) override {
        ProcessBinaryInstruction<std::greater_equal, FloatOperand>(instr);
    }

    void VisitIdInstruction(IdInstruction *instr) override {
        auto *dest = instr->GetDest();
        auto *op = instr->GetDest();
        sscp->values[dest] = sscp->values[op];

        if (sscp->values[dest] == LVT::CONSTANT) {
            sscp->constants[dest] = sscp->constants[op];
        }
    }

  private:
    SSCPTransformer *sscp;

    template <template <typename> typename Op, typename U,
              typename T = U::val_type>
    void ProcessBinaryInstruction(auto *instr, Op<T> op = Op<T>()) {
        auto *dest = instr->GetDest();
        auto *op0 = instr->GetOperand(0);
        auto *op1 = instr->GetOperand(1);

        if (sscp->values[op0] == LVT::INDETERMINABLE ||
            sscp->values[op1] == LVT::INDETERMINABLE) {
            sscp->values[dest] = LVT::INDETERMINABLE;
            return;
        }

        if (sscp->values[op0] == LVT::CONSTANT &&
            sscp->values[op1] == LVT::CONSTANT) {
            auto value = op(static_cast<U *>(sscp->constants[op0])->GetValue(),
                            static_cast<U *>(sscp->constants[op1])->GetValue());

            sscp->values[dest] = LVT::CONSTANT;

            using ret_type =
                std::invoke_result_t<decltype(op), typename U::val_type,
                                     typename U::val_type>;

            if constexpr (std::is_same_v<ret_type, typename U::val_type>) {
                sscp->constants[dest] = U::GetOperand(value);
            } else {
                sscp->constants[dest] = BoolOperand::GetOperand(value);
            }

            return;
        }
    }

    template <typename U> void ProcessMulInstruction(auto *instr) {
        auto *dest = instr->GetDest();
        auto *op0 = instr->GetOperand(0);
        auto *op1 = instr->GetOperand(1);

        if (sscp->values[op0] == LVT::INDETERMINABLE ||
            sscp->values[op1] == LVT::INDETERMINABLE) {
            sscp->values[dest] = LVT::INDETERMINABLE;
            return;
        }

        if (sscp->values[op0] == LVT::CONSTANT &&
            sscp->values[op1] == LVT::CONSTANT) {
            auto value = static_cast<U *>(sscp->constants[op0])->GetValue() *
                         static_cast<U *>(sscp->constants[op1])->GetValue();

            sscp->values[dest] = LVT::CONSTANT;
            sscp->constants[dest] = U::GetOperand(value);
            return;
        }

        assert(sscp->values[op0] == LVT::CONSTANT ||
               sscp->values[op1] == LVT::CONSTANT);

        auto *const_op = op0;
        if (sscp->values[op1] == LVT::CONSTANT) {
            const_op = op1;
        }

        if (static_cast<U *>(sscp->constants[const_op])->GetValue() ==
            static_cast<U::val_type>(0)) {
            sscp->values[dest] = LVT::CONSTANT;
            sscp->constants[dest] = U::GetOperand(static_cast<U::val_type>(0));
        }
    }

    template <typename U> void ProcessDivInstruction(auto *instr) {
        auto *dest = instr->GetDest();
        auto *op0 = instr->GetOperand(0);
        auto *op1 = instr->GetOperand(1);

        if (sscp->values[op0] == LVT::INDETERMINABLE ||
            sscp->values[op1] == LVT::INDETERMINABLE) {
            sscp->values[dest] = LVT::INDETERMINABLE;
            return;
        }

        if (sscp->values[op0] == LVT::CONSTANT &&
            sscp->values[op1] == LVT::CONSTANT) {
            // Divide by zero behaviour dependent on C++ impl.
            // behaviour.
            auto value = static_cast<U *>(sscp->constants[op0])->GetValue() /
                         static_cast<U *>(sscp->constants[op1])->GetValue();

            sscp->values[dest] = LVT::CONSTANT;
            sscp->constants[dest] = U::GetOperand(value);
            return;
        }

        assert(sscp->values[op0] == LVT::CONSTANT ||
               sscp->values[op1] == LVT::CONSTANT);

        if (sscp->values[op0] == LVT::CONSTANT &&
            static_cast<U *>(sscp->constants[op0])->GetValue() ==
                static_cast<U::val_type>(0)) {
            // 0/x = 0 for any real x give x != 0
            // since 0/0 is undefined, I can do anything!
            // therefore 0/0 = 0 (lol!)
            sscp->values[dest] = LVT::CONSTANT;
            sscp->constants[dest] = U::GetOperand(static_cast<U::val_type>(0));
        }
    }
};

} // namespace sc
