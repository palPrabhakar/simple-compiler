#include "function.hpp"
#include "json.hpp"
#include "operands.hpp"
#include "program.hpp"
#include <format>
#include <memory>
#include <optional>
#include <unordered_map>

namespace sc {
using sym_tbl = std::unordered_map<std::string, std::weak_ptr<OperandBase>>;

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
        auto operand = MakeOperand<RegOperand>(type);
        func->AddArgs(operand);
        symbols[name] = operand;
        func->AddOperand(std::move(operand));
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
