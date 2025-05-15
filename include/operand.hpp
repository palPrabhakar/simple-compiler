#pragma once

#include <cassert>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sc {
// clang-format off
enum class DataType {
    INT,
    BOOL,
    FLOAT,
    PTR,
    LABEL,
    VOID
};
// clang-format on

namespace ValType {
using INT = int64_t;
using FLOAT = double;
using BOOL = bool;
}; // namespace ValType

class Block;
class InstructionBase;

DataType GetDataTypeFromStr(std::string);
std::string GetStrDataType(DataType);
std::string GetPtrType(const std::vector<DataType> &ptr_chain);

class OperandBase {
  public:
    virtual ~OperandBase() = default;
    OperandBase(const OperandBase &) = delete;
    OperandBase &operator=(const OperandBase &) = delete;

    virtual std::unique_ptr<OperandBase> Clone() const = 0;

    void SetName(std::string _name) { name = _name; }

    void SetDef(InstructionBase *instr) { def = instr; }

    void SetUse(InstructionBase *instr) { uses.push_back(instr); }

    std::string GetStrType() const;

    DataType GetType() const { return type; }

    std::string GetName() const { return name; }

    InstructionBase *GetDef() const { return def; }

    InstructionBase *GetUse(size_t idx) const {
        assert(idx < uses.size());
        return uses[idx];
    }

  protected:
    OperandBase(DataType _type, std::string _name)
        : type(_type), name(std::move(_name)) {}

    DataType type;
    std::string name;

    // ssa-form single def
    InstructionBase *def;
    std::vector<InstructionBase *> uses;
};

class RegOperand : public OperandBase {
  public:
    RegOperand(DataType type, std::string name) : OperandBase(type, name) {}

    virtual std::unique_ptr<OperandBase> Clone() const override;
};

class PtrOperand final : public RegOperand {
  public:
    PtrOperand(std::string name) : RegOperand(DataType::PTR, name) {}

    std::unique_ptr<OperandBase> Clone() const override;

    void AppendPtrChain(DataType type) { ptr_chain.push_back(type); }

    const std::vector<DataType> &GetPtrChain() const { return ptr_chain; }

  private:
    std::vector<DataType> ptr_chain;
};

class LabelOperand final : public OperandBase {
  public:
    LabelOperand(std::string name) : OperandBase(DataType::LABEL, name) {}

    std::unique_ptr<OperandBase> Clone() const override {
        assert(false && "LabelOperand cannot be cloned\n");
        return nullptr;
    }

    void SetBlock(Block *blk) { block = blk; }

    Block *GetBlock() const { return block; }

  private:
    Block *block;
};

template <typename C> class ImmedOperand : public OperandBase {
    // TODO:
    // Ensure there is unique copy per ImmedOperand for a given value
  public:
    std::unique_ptr<OperandBase> Clone() const override {
        return static_cast<const C *>(this)->CloneImpl();
    }

  protected:
    ImmedOperand(DataType type, std::string name) : OperandBase(type, name) {}
};

class IntOperand final : public ImmedOperand<IntOperand> {
  public:
    using val_type = ValType::INT;
    IntOperand(std::string name, val_type val)
        : ImmedOperand<IntOperand>(DataType::INT, name), val(val) {}

    std::unique_ptr<OperandBase> CloneImpl() const;

    val_type GetValue() const { return val; }

  private:
    val_type val;
};

class FloatOperand final : public ImmedOperand<FloatOperand> {
  public:
    using val_type = ValType::FLOAT;
    FloatOperand(std::string name, val_type val)
        : ImmedOperand<FloatOperand>(DataType::FLOAT, name), val(val) {}

    std::unique_ptr<OperandBase> CloneImpl() const;

    val_type GetValue() const { return val; }

  private:
    val_type val;
};

class BoolOperand final : public ImmedOperand<BoolOperand> {
  public:
    using val_type = ValType::BOOL;
    BoolOperand(std::string name, val_type val)
        : ImmedOperand<BoolOperand>(DataType::BOOL, name), val(val) {}

    std::unique_ptr<OperandBase> CloneImpl() const;

    val_type GetValue() const { return val; }

  private:
    val_type val;
};

class UndefOperand : public OperandBase {
  public:
    static UndefOperand *GetUndefOperand() { return ptr; }

  private:
    static UndefOperand *ptr;
    static std::once_flag flag;

    std::unique_ptr<OperandBase> Clone() const override {
        assert(false && "LabelOperand cannot be cloned\n");
        return nullptr;
    }

    static void init() { ptr = new UndefOperand(); }

    UndefOperand() : OperandBase(DataType::VOID, "undef") {}
};

} // namespace sc
