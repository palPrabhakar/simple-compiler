#include "function.hpp"
#include "instructions.hpp"
#include "json.hpp"
#include "opcodes.hpp"
#include "operands.hpp"
#include "program.hpp"
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace sc {
using sym_tbl = std::unordered_map<std::string, std::weak_ptr<OperandBase>>;

template <typename T> static inline T GetJsonValue(sjp::Json json) {
    if constexpr (std::is_same_v<int, T>) {
        return static_cast<int>(json.Get<double>().value());
    } else {
        return json.Get<T>().value();
    }
}

std::unique_ptr<Function> ParseArguments(std::unique_ptr<Function> func,
                                         sjp::Json &args, sym_tbl &symbols) {
    // TODO:
    // Set register number
    for (size_t i = 0; i < args.Size(); ++i) {
        auto jop = args.Get(i).value();
        std::string name = jop.Get("name")->Get<std::string>().value();
        std::string type = jop.Get("type")->Get<std::string>().value();
        if (symbols.contains(name)) {
            throw std::runtime_error(
                std::format("Error parsing arguments of function {}. "
                            "Redeclaration of argument name {}\n",
                            func->GetName(), name));
        }
        auto operand = std::make_shared<RegOperand>(GetDataTypeFromStr(type));
        func->AddArgs(operand);
        symbols[name] = operand;
        func->AddOperand(std::move(operand));
    }
    return func;
}

template <typename T, DataType type>
static std::unique_ptr<Function>
BuildConstInstruction(std::unique_ptr<Function> func, sjp::Json &instr,
                      std::string type_str, sym_tbl &tbl) {
    // One single ImmedOperand represents every instance of that constant
    T value = GetJsonValue<T>(instr.Get("value").value());
    auto dest = instr.Get("dest")->Get<std::string>().value();
    std::string sym_name = "_$K_" + type_str + "_" + std::to_string(value);
    auto instr_ptr = std::make_unique<ConstInstruction>();
    if (tbl.contains(dest)) {
        // dest already exists
        instr_ptr->SetOperand(tbl[dest], 0);
    } else {
        // create new dest reg operand
        auto dest_oprnd = std::make_shared<RegOperand>(type);
        tbl[dest] = dest_oprnd;
        instr_ptr->SetOperand(dest_oprnd, 0);
        func->AddOperand(std::move(dest_oprnd));
    }
    if (tbl.contains(sym_name)) {
        // Const already exists
        instr_ptr->SetOperand(tbl[sym_name], 1);
    } else {
        // Create new Immed Operand
        auto src_oprnd = std::make_shared<ImmedOperand<T>>(type, value);
        tbl[sym_name] = src_oprnd;
        instr_ptr->SetOperand(src_oprnd, 1);
        func->AddOperand(std::move(src_oprnd));
    }
    return func;
}

std::unique_ptr<Function> MakeConstInstruction(std::unique_ptr<Function> func,
                                               sjp::Json &instr,
                                               sym_tbl &symbols) {
    auto type_str = instr.Get("type")->Get<std::string>().value();
    DataType type = GetDataTypeFromStr(type_str);
    switch (type) {
    case DataType::INT:
        return BuildConstInstruction<int, DataType::INT>(
            std::move(func), instr, std::move(type_str), symbols);
    case DataType::FLOAT:
        return BuildConstInstruction<double, DataType::FLOAT>(
            std::move(func), instr, std::move(type_str), symbols);
    case DataType::BOOL:
        return BuildConstInstruction<bool, DataType::BOOL>(
            std::move(func), instr, std::move(type_str), symbols);
    default:
        assert(false && "Unexpected type found in MakeConstInstruction\n");
        break;
    }
    return func;
}

std::unique_ptr<Function> ParseInstructions(std::unique_ptr<Function> func,
                                            sjp::Json &instr,
                                            sym_tbl &symbols) {
    auto opcode_str = instr.Get("op")->Get<std::string>().value();
    OpCode opcode = GetOpCodeFromStr(opcode_str);
    switch (opcode) {
    case OpCode::CONST:
        return MakeConstInstruction(std::move(func), instr, symbols);
    default:
        break;
    }
    return nullptr;
}

std::unique_ptr<Function> ParseBody(std::unique_ptr<Function> func,
                                    sjp::Json &instrs, sym_tbl &symbols) {
    for (size_t i = 0; i < instrs.Size(); ++i) {
        auto instr = instrs.Get(i).value();
        func = ParseInstructions(std::move(func), instr, symbols);
    }
    return func;
}

std::unique_ptr<Function> ParseFunction(sjp::Json jfunc) {
    sym_tbl symbols;
    auto func = std::make_unique<Function>(
        jfunc.Get("name")->Get<std::string>().value());
    auto args = jfunc.Get("args");
    if (args != std::nullopt) {
        func = ParseArguments(std::move(func), args.value(), symbols);
    }
    auto instrs = jfunc.Get("instrs");
    if (instrs == std::nullopt || instrs->Size() == 0) {
        throw std::runtime_error(std::format("Error parsing function {}. "
                                             "No valid instructions found.\n",
                                             func->GetName()));
    }
    func = ParseBody(std::move(func), instrs.value(), symbols);
    return func;
}

std::unique_ptr<Program> ParseProgram(sjp::Json jprogram) {
    auto program = std::make_unique<Program>();
    try {
        auto functions = jprogram.Get("functions");
        for (size_t i = 0; i < functions->Size(); ++i) {
            program->AddFunction(ParseFunction(functions->Get(i).value()));
        }
    } catch (std::bad_optional_access &err) {
        throw std::runtime_error("Unable to Parse Bril Progam\n");
    }
    return program;
}
} // namespace sc
