#include "instruction.hpp"
#include "operand.hpp"
#include <cassert>
#include <memory>
#include <ranges>

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

std::string GetPtrType(const std::vector<DataType> &ptr_chain) {
    std::string type;
    type += "ptr<";
    size_t count = 1;
    for (auto t : ptr_chain) {
        if (t == DataType::PTR) {
            ++count;
            type += "ptr<";
        } else {
            type += GetStrDataType(t);
        }
    }
    type += std::string(count, '>');
    return type;
}

void ReplaceUses(OperandBase *base, OperandBase *replacement) {
    for (auto i : std::views::iota(0ul, base->GetUsesSize())) {
        auto *use = base->GetUse(i);
        for (auto j : std::views::iota(0ul, use->GetOperandSize())) {
            if (use->GetOperand(j) == base) {
                use->SetOperand(replacement, j);
            }
        }
    }
}

std::string OperandBase::GetStrType() const {
    if (type == DataType::PTR) {
        return GetPtrType(static_cast<const PtrOperand *>(this)->GetPtrChain());
    } else {
        return GetStrDataType(type);
    }
}

std::unique_ptr<OperandBase> RegOperand::Clone() const {
    return std::make_unique<RegOperand>(type, name);
}

std::unique_ptr<OperandBase> PtrOperand::Clone() const {
    auto clone = std::make_unique<PtrOperand>(name);
    clone->ptr_chain = ptr_chain;
    return clone;
}

std::unique_ptr<OperandBase> IntOperand::CloneImpl() const {
    auto clone = std::make_unique<IntOperand>(name, val);
    return clone;
}

std::unique_ptr<OperandBase> FloatOperand::CloneImpl() const {
    auto clone = std::make_unique<FloatOperand>(name, val);
    return clone;
}

std::unique_ptr<OperandBase> BoolOperand::CloneImpl() const {
    auto clone = std::make_unique<BoolOperand>(name, val);
    return clone;
}

UndefOperand *UndefOperand::ptr = nullptr;

} // namespace sc
