#pragma once

#include <cassert>
#include <string>

namespace sc {

// clang-format off
enum class DataType {
    INT,
    BOOL,
    FLOAT,
    LABEL,
    VOID
};
// clang-format on

DataType GetDataTypeFromStr(std::string);
std::string GetStrDataType(DataType);

class OperandBase {
  public:
    virtual ~OperandBase() = default;

    DataType GetType() const { return type; }

    std::string GetName() const { return name; }

  protected:
    OperandBase(DataType _type, std::string _name)
        : type(_type), name(std::move(_name)) {}

  private:
    DataType type;
    std::string name;
};

class RegOperand : public OperandBase {
  public:
    RegOperand(DataType type, std::string name) : OperandBase(type, name) {}
};

class LabelOperand : public OperandBase {
  public:
    LabelOperand(std::string name) : OperandBase(DataType::LABEL, name) {}
};

template <typename T> class ImmedOperand : public OperandBase {
  public:
    ImmedOperand(DataType type, std::string name)
        : OperandBase(type, name), val(T{}) {}
    ImmedOperand(DataType type, std::string name, T val)
        : OperandBase(type, name), val(val) {}
    T GetValue() { return val; }

  private:
    T val;
};
} // namespace sc
