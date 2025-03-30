#pragma once

#include "instructions.hpp"
#include <memory>
#include <string>
#include <vector>

class Function {
  public:
    Function(std::string _name) : name(_name) {}

  private:
    std::vector<std::unique_ptr<InstructionBase>> instructions;
    std::string name;
};
