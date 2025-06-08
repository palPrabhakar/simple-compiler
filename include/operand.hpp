#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <mutex>
#include <span>
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
class OperandBase;

DataType GetDataTypeFromStr(std::string);
std::string GetStrDataType(DataType);
std::string GetPtrType(const std::vector<DataType> &ptr_chain);

void ReplaceUses(OperandBase *base, OperandBase *replacement);

class OperandBase {
  public:
    virtual ~OperandBase() = default;
    OperandBase(const OperandBase &) = delete;
    OperandBase &operator=(const OperandBase &) = delete;

    virtual std::shared_ptr<OperandBase> Clone() const = 0;

    /*
     * Name & Type
     *
     */
    void SetName(std::string _name) { name = _name; }

    std::string GetName() const { return name; }

    DataType GetType() const { return type; }

    std::string GetStrType() const;

    /*
     * Def
     */
    void SetDef(InstructionBase *instr) { def = instr; }

    InstructionBase *GetDef() const { return def; }

    /*
     * Use
     */
    void SetUse(InstructionBase *instr) { uses.push_back(instr); }

    size_t GetUsesSize() const { return uses.size(); }

    InstructionBase *GetUse(size_t idx) const {
        assert(idx < uses.size());
        return uses[idx];
    }

    std::span<InstructionBase *> GetUses() { return std::span(uses); }

    void RemoveUse(InstructionBase *instr) {
        auto it = std::find(uses.begin(), uses.end(), instr);
        if (it != uses.end()) {
            uses.erase(it);
        }
    }

  protected:
    OperandBase(DataType _type, std::string _name)
        : type(_type), name(std::move(_name)), def(nullptr) {}

    DataType type;
    std::string name;

    // ssa-form single def
    InstructionBase *def;
    std::vector<InstructionBase *> uses;
};

class RegOperand : public OperandBase {
  public:
    RegOperand(DataType type, std::string name) : OperandBase(type, name) {}

    virtual std::shared_ptr<OperandBase> Clone() const override;
};

class PtrOperand final : public RegOperand {
  public:
    PtrOperand(std::string name) : RegOperand(DataType::PTR, name) {}

    std::shared_ptr<OperandBase> Clone() const override;

    void AppendPtrChain(DataType type) { ptr_chain.push_back(type); }

    const std::vector<DataType> &GetPtrChain() const { return ptr_chain; }

  private:
    std::vector<DataType> ptr_chain;
};

class LabelOperand final : public OperandBase {
  public:
    LabelOperand(std::string name)
        : OperandBase(DataType::LABEL, name), block(nullptr) {}

    std::shared_ptr<OperandBase> Clone() const override {
        throw std::runtime_error("LabelOperand cannot be cloned.\n");
    }

    void SetBlock(Block *blk) { block = blk; }

    Block *GetBlock() const { return block; }

  private:
    Block *block;
};

template <typename C> class ImmedOperand : public OperandBase {
  protected:
    std::shared_ptr<OperandBase> Clone() const override {
        throw std::runtime_error("ImmedOperand cannot be cloned.\n");
    }

    ImmedOperand(DataType type, std::string name) : OperandBase(type, name) {}
};

class IntOperand final : public ImmedOperand<IntOperand> {
  public:
    using val_type = ValType::INT;
    static IntOperand *GetOperand(val_type value);

    val_type GetValue() const { return val; }

  protected:
    IntOperand(std::string name, val_type val)
        : ImmedOperand<IntOperand>(DataType::INT, name), val(val) {}

    val_type val;
};

class FloatOperand final : public ImmedOperand<FloatOperand> {
  public:
    using val_type = ValType::FLOAT;
    static FloatOperand *GetOperand(val_type value);

    val_type GetValue() const { return val; }

  protected:
    FloatOperand(std::string name, val_type val)
        : ImmedOperand<FloatOperand>(DataType::FLOAT, name), val(val) {}

    val_type val;
};

class BoolOperand final : public ImmedOperand<BoolOperand> {
  public:
    using val_type = ValType::BOOL;
    static BoolOperand *GetOperand(val_type value);

    val_type GetValue() const { return val; }

  protected:
    BoolOperand(std::string name, val_type val)
        : ImmedOperand<BoolOperand>(DataType::BOOL, name), val(val) {}

    val_type val;
};

// Undef Sentinel Singleton
class UndefOperand : public OperandBase {
  public:
    static std::shared_ptr<UndefOperand> GetUndefOperand() {
        std::call_once(flag, [] {
            ptr = std::shared_ptr<UndefOperand>(new UndefOperand());
        });
        return ptr;
    }

  private:
    static std::shared_ptr<UndefOperand> ptr;
    static std::once_flag flag;

    std::shared_ptr<OperandBase> Clone() const override {
        throw std::runtime_error("UndefOperand cannot be cloned.\n");
    }

    UndefOperand() : OperandBase(DataType::VOID, "__undef__") {}
};

// Void Sentinel Singleteon
class VoidOperand : public OperandBase {
  public:
    static std::shared_ptr<VoidOperand> GetVoidOperand() {
        std::call_once(flag, [] {
            ptr = std::shared_ptr<VoidOperand>(new VoidOperand());
        });
        return ptr;
    }

  private:
    static std::shared_ptr<VoidOperand> ptr;
    static std::once_flag flag;

    std::shared_ptr<OperandBase> Clone() const override {
        throw std::runtime_error("VoidOperand cannot be cloned.\n");
    }

    VoidOperand() : OperandBase(DataType::VOID, "__void__") {}
};

} // namespace sc
