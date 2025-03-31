#pragma once

#include <memory>
#include <type_traits>

namespace sc {

// clang-format off
enum class DataType {
    INT,
    BOOL,
    FLOAT
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

template <typename T> class ImmedOperand : public OperandBase {
  public:
    ImmedOperand(DataType _type) : OperandBase(_type), val(T{}) {}
    ImmedOperand(DataType _type, T _val) : OperandBase(_type), val(_val) {}
    T GetValue() { return val; }

  private:
    T val;
};

template <typename T>
concept OperandType = std::is_base_of_v<OperandBase, T>;

template <OperandType T>
std::shared_ptr<OperandBase> MakeOperand(std::string type_str) {
    DataType type = GetDataTypeFromStr(type_str);
    return std::make_shared<T>(type);
}
} // namespace sc
