#pragma once
#include "opcodes.hpp"
#include "operand.hpp"
#include <cassert>
#include <iostream>
#include <ostream>
#include <span>
#include <vector>

namespace sc {

class Block;
class InstructionBase;

OpCode GetOpCodeFromStr(std::string);

class InstructionBase {
  public:
    virtual ~InstructionBase() = default;

    virtual void Dump(std::ostream &out = std::cout) const = 0;

    virtual bool HasDest() const { return false; }

    void SetBlock(Block *blk) { block = blk; };

    void SetDest(OperandBase *oprnd) { dest = oprnd; }

    void SetOperand(OperandBase *oprnd) { operands.push_back(oprnd); }

    void SetOperand(OperandBase *oprnd, size_t idx) {
        assert(idx < operands.size());
        operands[idx] = oprnd;
    }

    OpCode GetOpCode() const { return opcode; }

    size_t GetOperandSize() const { return operands.size(); }

    OperandBase *GetDest() const { return dest; }

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
    InstructionBase(OpCode _opcode) : opcode(_opcode) {}
    OperandBase *dest;
    std::vector<OperandBase *> operands; // non-owning pointers

  private:
    OpCode opcode;
    Block *block;
};

// NewInstructionClass
class BinaryOperator : public InstructionBase {
  public:
    BinaryOperator(OpCode opcode) : InstructionBase(opcode) {}

    bool HasDest() const override { return true; }

    virtual bool Commutative() const { return false; }

    virtual bool NegateCommutative() const { return false; }

    virtual OpCode NegateCommutativityOp() const { return OpCode::NOP; }
};

class UnaryOperator : public InstructionBase {
  public:
    UnaryOperator(OpCode opcode) : InstructionBase(opcode) {}

    bool HasDest() const override { return true; }
};

// Arithmetic Instructions
class AddInstruction final : public BinaryOperator {
  public:
    AddInstruction() : BinaryOperator(OpCode::ADD) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }
};

class MulInstruction final : public BinaryOperator {
  public:
    MulInstruction() : BinaryOperator(OpCode::MUL) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }
};

class SubInstruction final : public BinaryOperator {
  public:
    SubInstruction() : BinaryOperator(OpCode::SUB) {}

    void Dump(std::ostream &out = std::cout) const override;
};

class DivInstruction final : public BinaryOperator {
  public:
    DivInstruction() : BinaryOperator(OpCode::DIV) {}

    void Dump(std::ostream &out = std::cout) const override;
};

// Comparison Instructions
class EqInstruction final : public BinaryOperator {
  public:
    EqInstruction() : BinaryOperator(OpCode::EQ) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }
};

class LtInstruction final : public BinaryOperator {
  public:
    LtInstruction() : BinaryOperator(OpCode::LT) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool NegateCommutative() const override { return true; }

    OpCode NegateCommutativityOp() const override { return OpCode::GT; }
};

class GtInstruction final : public BinaryOperator {
  public:
    GtInstruction() : BinaryOperator(OpCode::GT) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool NegateCommutative() const override { return true; }

    OpCode NegateCommutativityOp() const override { return OpCode::LT; }
};

class LeInstruction final : public BinaryOperator {
  public:
    LeInstruction() : BinaryOperator(OpCode::LE) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool NegateCommutative() const override { return true; }

    OpCode NegateCommutativityOp() const override { return OpCode::GE; }
};

class GeInstruction final : public BinaryOperator {
  public:
    GeInstruction() : BinaryOperator(OpCode::GE) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool NegateCommutative() const override { return true; }

    OpCode NegateCommutativityOp() const override { return OpCode::LE; }
};

// Logic Instructions
class AndInstruction final : public BinaryOperator {
  public:
    AndInstruction() : BinaryOperator(OpCode::AND) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }
};

class OrInstruction final : public BinaryOperator {
  public:
    OrInstruction() : BinaryOperator(OpCode::OR) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }
};

class NotInstruction final : public UnaryOperator {
  public:
    NotInstruction() : UnaryOperator(OpCode::NOT) {}

    void Dump(std::ostream &out = std::cout) const override;
};

// Control Instruction
class JmpInstruction final : public InstructionBase {
    // output_label = f(input_label)
    // Changes PC by some offset
  public:
    JmpInstruction() : InstructionBase(OpCode::JMP) {}

    void Dump(std::ostream &out = std::cout) const override;

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
    BranchInstruction() : InstructionBase(OpCode::BR) {}

    void Dump(std::ostream &out = std::cout) const override;

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
    CallInstruction() : InstructionBase(OpCode::CALL) {}

    void Dump(std::ostream &out = std::cout) const override;

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
    RetInstruction() : InstructionBase(OpCode::RET) {}

    void Dump(std::ostream &out = std::cout) const override;
};

class GetInstruction;

// SSA Instructions
class SetInstruction final : public InstructionBase {
  public:
    SetInstruction() : InstructionBase(OpCode::SET) {}

    void Dump(std::ostream &out = std::cout) const override;

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
    GetInstruction() : InstructionBase(OpCode::GET) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool HasDest() const override { return true; }

    void SetSetPair(SetInstruction *instr) { sets.push_back(instr); }

    size_t GetSetPairSize() const { return sets.size(); }

    SetInstruction *GetSetPair(size_t idx) {
        assert(idx < sets.size());
        return sets[idx];
    }

    std::span<SetInstruction *> GetSetPairs() { return std::span(sets); }

  private:
    // There is atleast two set instr per get instr
    std::vector<SetInstruction *> sets;
};

class UndefInstruction final : public UnaryOperator {
  public:
    UndefInstruction() : UnaryOperator(OpCode::UNDEF) {}

    void Dump(std::ostream &out = std::cout) const override;
};

// Memory Instructions
class AllocInstruction : public UnaryOperator {
  public:
    AllocInstruction() : UnaryOperator(OpCode::ALLOC) {}

    void Dump(std::ostream &out = std::cout) const override;
};

class FreeInstruction : public InstructionBase {
  public:
    FreeInstruction() : InstructionBase(OpCode::FREE) {}

    void Dump(std::ostream &out = std::cout) const override;
};

class LoadInstruction : public UnaryOperator {
  public:
    LoadInstruction() : UnaryOperator(OpCode::LOAD) {}

    void Dump(std::ostream &out = std::cout) const override;
};

class StoreInstruction : public InstructionBase {
  public:
    StoreInstruction() : InstructionBase(OpCode::STORE) {}

    void Dump(std::ostream &out = std::cout) const override;
};

class PtraddInstruction : public BinaryOperator {
  public:
    PtraddInstruction() : BinaryOperator(OpCode::PTRADD) {}

    void Dump(std::ostream &out = std::cout) const override;
};

// Floating-Point Arithmetic Instructions
class FAddInstruction final : public BinaryOperator {
  public:
    FAddInstruction() : BinaryOperator(OpCode::FADD) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }
};

class FMulInstruction final : public BinaryOperator {
  public:
    FMulInstruction() : BinaryOperator(OpCode::FMUL) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }
};

class FSubInstruction final : public BinaryOperator {
  public:
    FSubInstruction() : BinaryOperator(OpCode::FSUB) {}

    void Dump(std::ostream &out = std::cout) const override;
};

class FDivInstruction final : public BinaryOperator {
  public:
    FDivInstruction() : BinaryOperator(OpCode::FDIV) {}

    void Dump(std::ostream &out = std::cout) const override;
};

// Floating-Pointi Comparison Instructions
class FEqInstruction final : public BinaryOperator {
  public:
    FEqInstruction() : BinaryOperator(OpCode::FEQ) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool Commutative() const override { return true; }
};

class FLtInstruction final : public BinaryOperator {
  public:
    FLtInstruction() : BinaryOperator(OpCode::FLT) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool NegateCommutative() const override { return true; }

    OpCode NegateCommutativityOp() const override { return OpCode::FGT; }
};

class FGtInstruction final : public BinaryOperator {
  public:
    FGtInstruction() : BinaryOperator(OpCode::FGT) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool NegateCommutative() const override { return true; }

    OpCode NegateCommutativityOp() const override { return OpCode::FLT; }
};

class FLeInstruction final : public BinaryOperator {
  public:
    FLeInstruction() : BinaryOperator(OpCode::FLE) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool NegateCommutative() const override { return true; }

    OpCode NegateCommutativityOp() const override { return OpCode::FGE; }
};

class FGeInstruction final : public BinaryOperator {
  public:
    FGeInstruction() : BinaryOperator(OpCode::FGE) {}

    void Dump(std::ostream &out = std::cout) const override;

    bool NegateCommutative() const override { return true; }

    OpCode NegateCommutativityOp() const override { return OpCode::FLE; }
};

// Miscellaneous Instructions
class IdInstruction final : public UnaryOperator {
  public:
    IdInstruction() : UnaryOperator(OpCode::ID) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class ConstInstruction final : public UnaryOperator {
    // Src is ImmedOperand
  public:
    ConstInstruction() : UnaryOperator(OpCode::CONST) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class PrintInstruction final : public InstructionBase {
  public:
    PrintInstruction() : InstructionBase(OpCode::PRINT) {}
    void Dump(std::ostream &out = std::cout) const override;
};

class NopInstruction final : public InstructionBase {
  public:
    NopInstruction() : InstructionBase(OpCode::NOP) {}
    void Dump(std::ostream &out = std::cout) const override;
};

// Helper functions
// Only meaningful in SSA-form
inline void SetDestAndDef(InstructionBase *instr, OperandBase *oprnd) {
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
