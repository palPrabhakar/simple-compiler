#include "operands.hpp"
#include <cassert>
#include <format>

namespace sc {
DataType GetDataTypeFromStr(std::string type_str) {
    switch (type_str[0]) {
    case 66:
        return DataType::BOOL;
    case 70:
        return DataType::FLOAT;
    case 73:
        return DataType::INT;
    case 98:
        return DataType::BOOL;
    case 102:
        return DataType::FLOAT;
    case 108:
        return DataType::INT;
    default:
        assert("false" &&
               std::format("In file {} at line {} - GetDataTypeFromStr: "
                           "Invalid data type str\n",
                           __FILE__, __LINE__)
                   .c_str());
    }
    __builtin_unreachable();
}
} // namespace sc
