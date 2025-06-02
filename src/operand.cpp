#include "operand.hpp"
#include "instruction.hpp"
#include <cassert>
#include <memory>
#include <unordered_map>

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

void ReplaceUses(OperandBase *orig, OperandBase *repl) {
    // Need to make a copy here since the uses vector will be
    // re-written during the operation and the iterators will
    // be invalidated.
    std::vector<InstructionBase *> uses(orig->GetUses().begin(),
                                        orig->GetUses().end());
    for (auto *use : uses) {
        auto operands = use->GetOperands();
        auto it = std::find(operands.begin(), operands.end(), orig);
        if (it != operands.end()) {
            SetOperandAndUse(use, repl,
                             static_cast<size_t>(it - operands.begin()));
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

std::shared_ptr<OperandBase> RegOperand::Clone() const {
    return std::make_shared<RegOperand>(type, name);
}

std::shared_ptr<OperandBase> PtrOperand::Clone() const {
    auto clone = std::make_shared<PtrOperand>(name);
    clone->ptr_chain = ptr_chain;
    return clone;
}

IntOperand *IntOperand::GetOperand(val_type value) {
    static std::unordered_map<val_type, std::unique_ptr<IntOperand>> store;
    if (!store.contains(value)) {
        store.emplace(value, std::unique_ptr<IntOperand>(new IntOperand(
                                 "_$IK_" + std::to_string(value), value)));
    }
    return store[value].get();
}

FloatOperand *FloatOperand::GetOperand(val_type value) {
    static std::unordered_map<val_type, std::unique_ptr<FloatOperand>> store;
    if (!store.contains(value)) {
        store.emplace(value, std::unique_ptr<FloatOperand>(new FloatOperand(
                                 "_$FK_" + std::to_string(value), value)));
    }
    return store[value].get();
}

BoolOperand *BoolOperand::GetOperand(val_type value) {
    static std::unique_ptr<BoolOperand> store[2];
    if (store[value] == nullptr) {
        store[value] = std::unique_ptr<BoolOperand>(
            new BoolOperand(value ? "true" : "false", value));
    }
    return store[value].get();
}

std::shared_ptr<UndefOperand> UndefOperand::ptr = nullptr;
std::once_flag UndefOperand::flag;

std::shared_ptr<VoidOperand> VoidOperand::ptr = nullptr;
std::once_flag VoidOperand::flag;

} // namespace sc
