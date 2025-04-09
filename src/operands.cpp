#include "operands.hpp"
#include <cassert>
#include <iostream>

namespace sc {
DataType GetDataTypeFromStr(std::string type_str) {
    switch ((int)type_str[0]) {
    case 98:
        return DataType::BOOL;
    case 102:
        return DataType::FLOAT;
    case 105:
        return DataType::INT;
    default:
        assert(false && "GetDataTypeFromStr: Invalid data type str\n");
    }
    __builtin_unreachable();
}

std::string GetStrDataType(DataType type) {
    switch (type) {
    case DataType::INT:
        return "int";
    case DataType::BOOL:
        return "bool";
    case DataType::FLOAT:
        return "float";
    case DataType::LABEL:
        return "label";
    default:
        assert(false && "GetStrDataType: Invalid data type\n");
    }
    __builtin_unreachable();
}

} // namespace sc
