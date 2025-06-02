#pragma once
#include "opcodes.hpp"
#include "operand.hpp"
#include <cassert>
#include <iostream>
#include <ostream>
#include <span>
#include <vector>
#include <memory>

namespace sc {

class Block;
class InstructionBase;
class InstructionVisitor;

Opcode GetOpcodeFromStr(std::string);

class InstructionBase {
  public:
    virtual ~InstructionBase() = default;

    virtual void Dump(std::ostream &out = std::cout) const = 0;

    virtual bool HasDest() const { return false; }

    virtual void Visit(InstructionVisitor *visitor) = 0;

    void SetBlock(Block *blk) { block = blk; };

    void SetDest(std::shared_ptr<OperandBase> oprnd) { dest = std::move(oprnd); }

    void SetOperand(OperandBase *oprnd) { operands.push_back(oprnd); }

    void SetOperand(OperandBase *oprnd, size_t idx) {
        assert(idx < operands.size());
        operands[idx] = oprnd;
    }

    Opcode GetOpcode() const { return opcode; }

    size_t GetOperandSize() const { return operands.size(); }

    OperandBase *GetDest() const { return dest.get(); }

    std::shared_ptr<OperandBase> CopyDest() const { return dest; }

    std::shared_ptr<OperandBase> ReleaseDest() const { return std::move(dest); }

    OperandBase *GetOperand(size_t idx) const {
        assert(idx < operands.size());
        return operands[idx];
    }

    std::span<OperandBase *> GetOperands() {
        return std::span<OperandBase *>(operands);
    }

    Block *GetBlock() const {
        assert(block != nullptr && "No owning block.\n");
        return block;
    }

  protected:
    InstructionBase(Opcode _opcode) : opcode(_opcode) {}
    // Since the same instruction class is used to represent
    // the IR in both SSA and non-SSA form. In case of SSA
    // form the reference count must be 1.
    std::shared_ptr<OperandBase> dest;
    std::vector<OperandBase *> operands; // non-owning pointers

  private:
    Opcode opcode;
    Block *block;
};

// NewInstructionClass
class BinaryOperator : public InstructionBase {
  public:
    BinaryOperator(Opcode Opcode) : InstructionBase(Opcode) {}

    bool HasDest() const override { return true; }

    virtual bool Commutative() const { return false; }

    virtual bool NegateCommutative() const { return false; }

    virtual Opcode NegateCommutativityOp() const { return Opcode::NOP; }
};

class UnaryOperator : public InstructionBase {
  public:
    UnaryOperator(Opcode Opcode) : InstructionBase(Opcode) {}

    bool HasDest() const override { return true; }
};

// Arithmetic Instructions
class AddInstruction final : public BinaryOperator {
  public:
    AddInstruction() : BinaryOperator(Opcode::ADD) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool Commutative() const override { return true; }
};

class MulInstruction final : public BinaryOperator {
  public:
    MulInstruction() : BinaryOperator(Opcode::MUL) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool Commutative() const override { return true; }
};

class SubInstruction final : public BinaryOperator {
  public:
    SubInstruction() : BinaryOperator(Opcode::SUB) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class DivInstruction final : public BinaryOperator {
  public:
    DivInstruction() : BinaryOperator(Opcode::DIV) {}

    void Visit(InstructionVisitor *visitor) override;

    void Dump(std::ostream &out = std::cout) const override;
};

// Comparison Instructions
class EqInstruction final : public BinaryOperator {
  public:
    EqInstruction() : BinaryOperator(Opcode::EQ) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool Commutative() const override { return true; }
};

class LtInstruction final : public BinaryOperator {
  public:
    LtInstruction() : BinaryOperator(Opcode::LT) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool NegateCommutative() const override { return true; }

    Opcode NegateCommutativityOp() const override { return Opcode::GT; }
};

class GtInstruction final : public BinaryOperator {
  public:
    GtInstruction() : BinaryOperator(Opcode::GT) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool NegateCommutative() const override { return true; }

    Opcode NegateCommutativityOp() const override { return Opcode::LT; }
};

class LeInstruction final : public BinaryOperator {
  public:
    LeInstruction() : BinaryOperator(Opcode::LE) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool NegateCommutative() const override { return true; }

    Opcode NegateCommutativityOp() const override { return Opcode::GE; }
};

class GeInstruction final : public BinaryOperator {
  public:
    GeInstruction() : BinaryOperator(Opcode::GE) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool NegateCommutative() const override { return true; }

    Opcode NegateCommutativityOp() const override { return Opcode::LE; }
};

// Logic Instructions
class AndInstruction final : public BinaryOperator {
  public:
    AndInstruction() : BinaryOperator(Opcode::AND) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool Commutative() const override { return true; }
};

class OrInstruction final : public BinaryOperator {
  public:
    OrInstruction() : BinaryOperator(Opcode::OR) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool Commutative() const override { return true; }
};

class NotInstruction final : public UnaryOperator {
  public:
    NotInstruction() : UnaryOperator(Opcode::NOT) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

// Control Instruction
class JmpInstruction final : public InstructionBase {
    // output_label = f(input_label)
    // Changes PC by some offset
  public:
    JmpInstruction() : InstructionBase(Opcode::JMP) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool HasDest() const override { return false; }

    void SetJmpDest(LabelOperand *lbl) { jmp_lbl = lbl; }

    LabelOperand *GetJmpDest() const { return jmp_lbl; }

  private:
    LabelOperand *jmp_lbl;
};

class BranchInstruction final : public InstructionBase {
    // One source and two destinations
    // Source - RegOperand
    // Dest - LabelOperand
    // output_label = f(cond, input_label{0, 1})
    // Changes PC by some offset
  public:
    BranchInstruction() : InstructionBase(Opcode::BR) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool HasDest() const override { return false; }

    void SetTrueDest(LabelOperand *lbl) { true_lbl = lbl; }

    void SetFalseDest(LabelOperand *lbl) { false_lbl = lbl; }

    LabelOperand *GetTrueDest() const { return true_lbl; }

    LabelOperand *GetFalseDest() const { return false_lbl; }

  private:
    LabelOperand *true_lbl;
    LabelOperand *false_lbl;
};

class CallInstruction final : public InstructionBase {
    // First operand is dest
    // Rest are arguments
  public:
    CallInstruction() : InstructionBase(Opcode::CALL) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool HasDest() const override { return ret_val; }

    void SetFuncName(std::string fname) { func = fname; }

    void SetRetVal(bool val) { ret_val = val; }

    std::string GetFuncName() const { return func; }

  private:
    std::string func;
    bool ret_val;
};

class RetInstruction final : public InstructionBase {
  public:
    RetInstruction() : InstructionBase(Opcode::RET) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class GetInstruction;

// SSA Instructions
class SetInstruction final : public InstructionBase {
  public:
    SetInstruction() : InstructionBase(Opcode::SET) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    void SetShadow(OperandBase *oprnd) { shadow = oprnd; }

    OperandBase *GetShadow() const { return shadow; }

    void SetGetPair(GetInstruction *instr) { get = instr; }

    GetInstruction *GetGetPair() const { return get; }

  private:
    OperandBase *shadow;
    // There can be only one get instr per set instr
    GetInstruction *get;
};

class GetInstruction final : public InstructionBase {
  public:
    GetInstruction() : InstructionBase(Opcode::GET) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool HasDest() const override { return true; }

    void SetShadow(OperandBase *oprnd) { shadow = oprnd; }

    OperandBase *GetShadow() const { return shadow; }

    void SetSetPair(SetInstruction *instr) { sets.push_back(instr); }

    size_t GetSetPairSize() const { return sets.size(); }

    SetInstruction *GetSetPair(size_t idx) {
        assert(idx < sets.size());
        return sets[idx];
    }

    std::span<SetInstruction *> GetSetPairs() { return std::span(sets); }

  private:
    OperandBase *shadow;
    // There is atleast two set instr per get instr
    std::vector<SetInstruction *> sets;
};

class UndefInstruction final : public UnaryOperator {
  public:
    UndefInstruction() : UnaryOperator(Opcode::UNDEF) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

// Memory Instructions
class AllocInstruction : public UnaryOperator {
  public:
    AllocInstruction() : UnaryOperator(Opcode::ALLOC) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class FreeInstruction : public InstructionBase {
  public:
    FreeInstruction() : InstructionBase(Opcode::FREE) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class LoadInstruction : public UnaryOperator {
  public:
    LoadInstruction() : UnaryOperator(Opcode::LOAD) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class StoreInstruction : public InstructionBase {
  public:
    StoreInstruction() : InstructionBase(Opcode::STORE) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class PtraddInstruction : public BinaryOperator {
  public:
    PtraddInstruction() : BinaryOperator(Opcode::PTRADD) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

// Floating-Point Arithmetic Instructions
class FAddInstruction final : public BinaryOperator {
  public:
    FAddInstruction() : BinaryOperator(Opcode::FADD) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool Commutative() const override { return true; }
};

class FMulInstruction final : public BinaryOperator {
  public:
    FMulInstruction() : BinaryOperator(Opcode::FMUL) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool Commutative() const override { return true; }
};

class FSubInstruction final : public BinaryOperator {
  public:
    FSubInstruction() : BinaryOperator(Opcode::FSUB) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class FDivInstruction final : public BinaryOperator {
  public:
    FDivInstruction() : BinaryOperator(Opcode::FDIV) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

// Floating-Pointi Comparison Instructions
class FEqInstruction final : public BinaryOperator {
  public:
    FEqInstruction() : BinaryOperator(Opcode::FEQ) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }

    void Visit(InstructionVisitor *visitor) override;
};

class FLtInstruction final : public BinaryOperator {
  public:
    FLtInstruction() : BinaryOperator(Opcode::FLT) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool NegateCommutative() const override { return true; }

    Opcode NegateCommutativityOp() const override { return Opcode::FGT; }
};

class FGtInstruction final : public BinaryOperator {
  public:
    FGtInstruction() : BinaryOperator(Opcode::FGT) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool NegateCommutative() const override { return true; }

    Opcode NegateCommutativityOp() const override { return Opcode::FLT; }
};

class FLeInstruction final : public BinaryOperator {
  public:
    FLeInstruction() : BinaryOperator(Opcode::FLE) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool NegateCommutative() const override { return true; }

    Opcode NegateCommutativityOp() const override { return Opcode::FGE; }
};

class FGeInstruction final : public BinaryOperator {
  public:
    FGeInstruction() : BinaryOperator(Opcode::FGE) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool NegateCommutative() const override { return true; }

    Opcode NegateCommutativityOp() const override { return Opcode::FLE; }
};

// Miscellaneous Instructions
class IdInstruction final : public UnaryOperator {
  public:
    IdInstruction() : UnaryOperator(Opcode::ID) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class ConstInstruction final : public UnaryOperator {
    // Src is ImmedOperand
  public:
    ConstInstruction() : UnaryOperator(Opcode::CONST) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class PrintInstruction final : public InstructionBase {
  public:
    PrintInstruction() : InstructionBase(Opcode::PRINT) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

class NopInstruction final : public InstructionBase {
  public:
    NopInstruction() : InstructionBase(Opcode::NOP) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;
};

// Internal
class GetArgInstruction final : public InstructionBase {
  public:
    GetArgInstruction() : InstructionBase(Opcode::GETARG) {}

    void Dump(std::ostream &out = std::cout) const override;

    void Visit(InstructionVisitor *visitor) override;

    bool HasDest() const override { return true; }
};

// Helper functions
// Only meaningful in SSA-form
inline void SetDestAndDef(InstructionBase *instr, std::shared_ptr<OperandBase> oprnd) {
    oprnd->SetDef(instr);
    instr->SetDest(oprnd);
}

inline void SetOperandAndUse(InstructionBase *instr, OperandBase *oprnd) {
    oprnd->SetUse(instr);
    instr->SetOperand(oprnd);
}

inline void SetOperandAndUse(InstructionBase *instr, OperandBase *oprnd,
                             size_t idx) {
    auto *op = instr->GetOperand(idx);
    op->RemoveUse(instr);
    oprnd->SetUse(instr);
    instr->SetOperand(oprnd, idx);
}
} // namespace sc
