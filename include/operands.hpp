#pragma once

// clang-format off
enum class OperandT {
    INT,
    BOOL,
    FLOAT
};
// clang-format on

class OperandBase {
  public:
    virtual void get_value();
    virtual ~OperandBase();

  protected:
    OperandBase() = delete;
};
