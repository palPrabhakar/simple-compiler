#pragma once
#include "opcodes.hpp"
#include "operand.hpp"
#include <cassert>
#include <iostream>
#include <ostream>
#include <vector>

namespace sc {

class Block;

OpCode GetOpCodeFromStr(std::string);

class InstructionBase {
  public:
    virtual ~InstructionBase() = default;
    virtual void Dump(std::ostream &out = std::cout) const = 0;
    OpCode GetOpCode() const { return opcode; }
    size_t GetOperandSize() const { return operands.size(); }

    void SetBlock(Block *blk) { block = blk; };

    void SetOperand(OperandBase *oprnd) { operands.push_back(oprnd); }

    void SetOperand(OperandBase *oprnd, size_t idx) {
        assert(idx < operands.size());
        operands[idx] = oprnd;
    }

    OperandBase *GetOperand(size_t idx) const {
        assert(idx < operands.size());
        return operands[idx];
    }

    Block *GetBlock() const {
        assert(block != nullptr && "No owning block.\n");
        return block;
    }

    static constexpr size_t OP_SIZE = 0;

  protected:
    InstructionBase(OpCode _opcode) : opcode(_opcode) {}
    std::vector<OperandBase *> operands; // non-owning pointers

  private:
    OpCode opcode;
    Block *block;
};

// Arithmetic Instructions
class AddInstruction final : public InstructionBase {
  public:
    AddInstruction() : InstructionBase(OpCode::ADD) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class MulInstruction final : public InstructionBase {
  public:
    MulInstruction() : InstructionBase(OpCode::MUL) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class SubInstruction final : public InstructionBase {
  public:
    SubInstruction() : InstructionBase(OpCode::SUB) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class DivInstruction final : public InstructionBase {
  public:
    DivInstruction() : InstructionBase(OpCode::DIV) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Comparison Instructions
class EqInstruction final : public InstructionBase {
  public:
    EqInstruction() : InstructionBase(OpCode::EQ) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class LtInstruction final : public InstructionBase {
  public:
    LtInstruction() : InstructionBase(OpCode::LT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class GtInstruction final : public InstructionBase {
  public:
    GtInstruction() : InstructionBase(OpCode::GT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class LeInstruction final : public InstructionBase {
  public:
    LeInstruction() : InstructionBase(OpCode::LE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class GeInstruction final : public InstructionBase {
  public:
    GeInstruction() : InstructionBase(OpCode::GE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Logic Instructions
class AndInstruction final : public InstructionBase {
  public:
    AndInstruction() : InstructionBase(OpCode::AND) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class OrInstruction final : public InstructionBase {
  public:
    OrInstruction() : InstructionBase(OpCode::OR) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class NotInstruction final : public InstructionBase {
  public:
    NotInstruction() : InstructionBase(OpCode::NOT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

// Control Instruction
class JmpInstruction final : public InstructionBase {
    // output_label = f(input_label)
    // Changes PC by some offset
  public:
    JmpInstruction() : InstructionBase(OpCode::JMP) {}
    void Dump(std::ostream &out = std::cout) const override;
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
};

class CallInstruction final : public InstructionBase {
    // First operand is dest
    // Rest are arguments
  public:
    CallInstruction() : InstructionBase(OpCode::CALL) {}
    void Dump(std::ostream &out = std::cout) const override;
    void SetFuncName(std::string fname) { func = fname; }
    void SetRetVal(bool val) { ret_val = val; }
    std::string GetFuncName() const { return func; }
    bool GetRetVal() const { return ret_val; }

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

    void SetGetPair(GetInstruction *instr) { get = instr; }

    GetInstruction *GetGetPair() const { return get; }

  private:
    // There can be only one get instr per set instr
    GetInstruction *get;
};

class GetInstruction final : public InstructionBase {
  public:
    GetInstruction() : InstructionBase(OpCode::GET) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 0;

    void SetSetPair(SetInstruction *instr) { sets.push_back(instr); }

    size_t GetSetPairSize() const { return sets.size(); }

    SetInstruction *GetSetPair(size_t idx) {
        assert(idx < sets.size());
        return sets[idx];
    }

  private:
    // There is atleast two set instr per get instr
    std::vector<SetInstruction *> sets;
};

class UndefInstruction final : public InstructionBase {
  public:
    UndefInstruction() : InstructionBase(OpCode::UNDEF) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

// Memory Instructions
class AllocInstruction : public InstructionBase {
  public:
    AllocInstruction() : InstructionBase(OpCode::ALLOC) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

class FreeInstruction : public InstructionBase {
  public:
    FreeInstruction() : InstructionBase(OpCode::FREE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

class LoadInstruction : public InstructionBase {
  public:
    LoadInstruction() : InstructionBase(OpCode::LOAD) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

class StoreInstruction : public InstructionBase {
  public:
    StoreInstruction() : InstructionBase(OpCode::STORE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class PtraddInstruction : public InstructionBase {
  public:
    PtraddInstruction() : InstructionBase(OpCode::PTRADD) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Floating-Point Arithmetic Instructions
class FAddInstruction final : public InstructionBase {
  public:
    FAddInstruction() : InstructionBase(OpCode::FADD) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FMulInstruction final : public InstructionBase {
  public:
    FMulInstruction() : InstructionBase(OpCode::FMUL) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FSubInstruction final : public InstructionBase {
  public:
    FSubInstruction() : InstructionBase(OpCode::FSUB) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FDivInstruction final : public InstructionBase {
  public:
    FDivInstruction() : InstructionBase(OpCode::FDIV) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Floating-Pointi Comparison Instructions
class FEqInstruction final : public InstructionBase {
  public:
    FEqInstruction() : InstructionBase(OpCode::FEQ) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FLtInstruction final : public InstructionBase {
  public:
    FLtInstruction() : InstructionBase(OpCode::FLT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FGtInstruction final : public InstructionBase {
  public:
    FGtInstruction() : InstructionBase(OpCode::FGT) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FLeInstruction final : public InstructionBase {
  public:
    FLeInstruction() : InstructionBase(OpCode::FLE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

class FGeInstruction final : public InstructionBase {
  public:
    FGeInstruction() : InstructionBase(OpCode::FGE) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 2;
};

// Miscellaneous Instructions
class IdInstruction final : public InstructionBase {
  public:
    IdInstruction() : InstructionBase(OpCode::ID) {}
    void Dump(std::ostream &out = std::cout) const override;
    static constexpr size_t OP_SIZE = 1;
};

class ConstInstruction final : public InstructionBase {
    // Src is ImmedOperand
  public:
    ConstInstruction() : InstructionBase(OpCode::CONST) {}
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
} // namespace sc
