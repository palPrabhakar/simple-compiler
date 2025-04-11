#pragma once
#include "opcodes.hpp"
#include "operands.hpp"
#include <cassert>
#include <iostream>
#include <ostream>
#include <ranges>
#include <vector>

namespace sc {

OpCode GetOpCodeFromStr(std::string);

#define PRINT_HELPER3(name)                                                    \
    auto dest = operands[0];                                                   \
    auto src0 = operands[1];                                                   \
    auto src1 = operands[2];                                                   \
    auto type = GetStrDataType(dest->GetType());                               \
    out << dest->GetName() << ":"                                              \
        << " " << type << " = " #name " " << src0->GetName() << " "            \
        << src1->GetName() << ";\n";

#define PRINT_HELPER2(name)                                                    \
    auto dest = operands[0];                                                   \
    auto src0 = operands[1];                                                   \
    auto type = GetStrDataType(dest->GetType());                               \
    out << dest->GetName() << ":"                                              \
        << " " << type << " = " #name " " << src0->GetName() << ";\n";

class InstructionBase {
  public:
    virtual ~InstructionBase() = default;

    virtual void Dump(std::ostream &out = std::cout) = 0;

    virtual void SetOperand(OperandBase *oprnd, size_t idx) {
        if (idx < operands.size()) {
            operands[idx] = std::move(oprnd);
        } else {
            operands.push_back(std::move(oprnd));
        }
    }

    OperandBase *GetOperand(size_t idx) {
        assert(idx < operands.size());
        return operands[idx];
    }

    OpCode GetOpCode() const { return opcode; }

  protected:
    InstructionBase(OpCode _opcode) : opcode(_opcode) {}
    std::vector<OperandBase *> operands; // non-owning pointers

  private:
    OpCode opcode;
};

class UnaryInstruction : public InstructionBase {
  public:
    UnaryInstruction(OpCode opcode) : InstructionBase(opcode) {}

    virtual void Dump(std::ostream &out = std::cout) override = 0;

    virtual void SetOperand(OperandBase *oprnd, size_t idx) override {
        assert(idx == 0 && "UnaryInstruction::SetOperand invalid index\n");
        InstructionBase::SetOperand(std::move(oprnd), 0);
    }
};

class BinaryInstruction : public InstructionBase {
  public:
    BinaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

    virtual void Dump(std::ostream &out = std::cout) override = 0;

    virtual void SetOperand(OperandBase *oprnd, size_t idx) override {
        assert((idx == 0 || idx == 1) &&
               "BinaryInstruction::SetOperand invalid index\n");
        InstructionBase::SetOperand(std::move(oprnd), idx);
    }
};

class TernaryInstruction : public InstructionBase {
  public:
    TernaryInstruction(OpCode _op_code) : InstructionBase(_op_code) {}

    virtual void Dump(std::ostream &out = std::cout) override = 0;

    virtual void SetOperand(OperandBase *oprnd, size_t idx) override {
        assert((idx == 0 || idx == 1 || idx == 2) &&
               "BinaryInstruction::SetOperand invalid index\n");
        InstructionBase::SetOperand(std::move(oprnd), idx);
    }
};

// Arithmetic Instructions
class AddInstruction final : public TernaryInstruction {
  public:
    AddInstruction() : TernaryInstruction(OpCode::ADD) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(add) }
};

class MulInstruction final : public TernaryInstruction {
  public:
    MulInstruction() : TernaryInstruction(OpCode::MUL) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(mul) }
};

class SubInstruction final : public TernaryInstruction {
  public:
    SubInstruction() : TernaryInstruction(OpCode::SUB) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(sub) }
};

class DivInstruction final : public TernaryInstruction {
  public:
    DivInstruction() : TernaryInstruction(OpCode::DIV) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(div) }
};

// Comparison Instructions
class EqInstruction final : public TernaryInstruction {
  public:
    EqInstruction() : TernaryInstruction(OpCode::EQ) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(eq) }
};

class LtInstruction final : public TernaryInstruction {
  public:
    LtInstruction() : TernaryInstruction(OpCode::LT) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(lt) }
};

class GtInstruction final : public TernaryInstruction {
  public:
    GtInstruction() : TernaryInstruction(OpCode::GT) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(gt) }
};

class LeInstruction final : public TernaryInstruction {
  public:
    LeInstruction() : TernaryInstruction(OpCode::LE) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(le) }
};

class GeInstruction final : public TernaryInstruction {
  public:
    GeInstruction() : TernaryInstruction(OpCode::GE) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(ge) }
};

// Logic Instructions
class AndInstruction final : public TernaryInstruction {
  public:
    AndInstruction() : TernaryInstruction(OpCode::AND) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(and) }
};

class OrInstruction final : public TernaryInstruction {
  public:
    OrInstruction() : TernaryInstruction(OpCode::OR) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER3(or) }
};

class NotInstruction final : public BinaryInstruction {
  public:
    NotInstruction() : BinaryInstruction(OpCode::NOT) {}

    // clang-format off
    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER2(not) }
    // clang-format on
};

// Control Instruction
class JmpInstruction final : public UnaryInstruction {
  public:
    JmpInstruction() : UnaryInstruction(OpCode::JMP) {}

    void Dump(std::ostream &out = std::cout) override {
        out << "jmp ." << operands[0]->GetName() << ";\n";
    }
};

class BranchInstruction final : public TernaryInstruction {
    // One source and two destinations
    // Source - RegOperand
    // Dest - LabelOperand
  public:
    BranchInstruction() : TernaryInstruction(OpCode::BR) {}

    void Dump(std::ostream &out = std::cout) override {
        out << "br " << operands[2]->GetName() << " ." << operands[0]->GetName()
            << " ." << operands[1]->GetName() << ";\n";
    }
};

class CallInstruction final : public InstructionBase {
    // First operand is dest
    // Rest are arguments
  public:
    CallInstruction() : InstructionBase(OpCode::CALL) {}

    void Dump(std::ostream &out = std::cout) override {
        out << operands[0]->GetName() << ":"
            << " " << GetStrDataType(operands[0]->GetType()) << " = call @"
            << func;
        for (size_t i : std::views::iota(1UL, operands.size())) {
            out << " " << operands[i]->GetName();
        }
        out << ";\n";
    }

    void SetFuncName(std::string fname) { func = fname; }

    std::string GetFuncName() const { return func; }

  private:
    std::string func;
};

class RetInstruction final : public UnaryInstruction {
  public:
    RetInstruction() : UnaryInstruction(OpCode::RET) {}

    void Dump(std::ostream &out = std::cout) override {
        out << "ret " << operands[0]->GetName() << ";\n";
    }
};

// Miscellaneous Instructions
class IdInstruction final : public BinaryInstruction {
  public:
    IdInstruction() : BinaryInstruction(OpCode::ID) {}

    void Dump(std::ostream &out = std::cout) override { PRINT_HELPER2(id) }
};

class ConstInstruction final : public BinaryInstruction {
    // Src is ImmedOperand
  public:
    ConstInstruction() : BinaryInstruction(OpCode::CONST) {}

    void Dump(std::ostream &out = std::cout) override {
        auto dest = operands[0];
        auto src = operands[1];
        auto type = GetStrDataType(dest->GetType());
        out << dest->GetName() << ":"
            << " " << type << " = const ";
        switch (src->GetType()) {
        case DataType::INT: {
            auto int_src = static_cast<ImmedOperand<int> *>(src);
            out << int_src->GetValue();
        } break;
        case DataType::FLOAT:
            assert(false && "todo float\n");
            break;
        case DataType::BOOL: {
            auto bool_src = static_cast<ImmedOperand<bool> *>(src);
            out << (bool_src->GetValue() ? "true" : "false");
        } break;
        default:
            assert(false);
        }
        out << ";\n";
    }
};

class PrintInstruction final : public InstructionBase {
  public:
    PrintInstruction() : InstructionBase(OpCode::PRINT) {}

    void Dump(std::ostream &out = std::cout) override {
        out << "print";
        for (auto op : operands) {
            out << " " << op->GetName();
        }
        out << ";\n";
    }
};

class NopInstruction final : public InstructionBase {
  public:
    NopInstruction() : InstructionBase(OpCode::NOP) {}

    void Dump(std::ostream &out = std::cout) override { out << "nop;\n"; }
};

class LabelInstruction final : public UnaryInstruction {
  public:
    LabelInstruction() : UnaryInstruction(OpCode::LABEL) {}

    void Dump(std::ostream &out = std::cout) override {
        out << "." << operands[0]->GetName() << ":\n";
    }
};

} // namespace sc
