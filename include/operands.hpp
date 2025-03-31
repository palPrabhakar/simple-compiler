#pragma once

#include <cassert>
#include <memory>
#include <type_traits>

namespace sc {

// clang-format off
enum class DataType {
    INT,
    BOOL,
    FLOAT,
    LABEL
};
// clang-format on

DataType GetDataTypeFromStr(std::string);

class OperandBase {
  public:
    virtual ~OperandBase() = default;

  protected:
    OperandBase(DataType _type) : type(_type) {}

  private:
    DataType type;
};

class RegOperand : public OperandBase {
  public:
    RegOperand(DataType _type) : OperandBase(_type) {}
};

class LabelOperand : public OperandBase {
  public:
    LabelOperand(DataType _type = DataType::LABEL) : OperandBase(_type) {
        assert(_type == DataType::LABEL &&
               "DataType not LABEL for LabelOperand\n");
    }
};

template <typename T> class ImmedOperand : public OperandBase {
  public:
    ImmedOperand(DataType _type) : OperandBase(_type), val(T{}) {}
    ImmedOperand(DataType _type, T _val) : OperandBase(_type), val(_val) {}
    T GetValue() { return val; }

  private:
    T val;
};
} // namespace sc
