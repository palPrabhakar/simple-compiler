#pragma once

#include "parser.hpp"
// sc includes
#include "operand.hpp"
#include "program.hpp"
// system includes
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>

namespace sc {
using OprndTbl = std::unordered_map<std::string, OperandBase *>;
using LabelTbl = std::unordered_map<std::string, LabelOperand *>;
using FuncPtr = std::unique_ptr<Function>;

class BrilParser {
  public:
    BrilParser(std::istream &program) {
        auto json_parser = sjp::Parser(program);
        data = json_parser.Parse();
    }

    std::unique_ptr<Program> ParseProgram();

  private:
    sjp::Json data;
    OprndTbl operands;
    LabelTbl labels;
    bool check;

    FuncPtr ParseFunction(sjp::Json &jfunc);
    FuncPtr ParseArguments(FuncPtr func, sjp::Json &args);
    FuncPtr ParseBody(FuncPtr func, sjp::Json &instrs);
    FuncPtr ParseInstructions(FuncPtr func, sjp::Json &instr);

    FuncPtr MakeNewBlock(FuncPtr func, std::string name);
    FuncPtr MakeJmpInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr MakeBranchInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr MakeCallInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr MakeConstInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr MakePrintInstruction(FuncPtr func, sjp::Json &instr);

    template <typename T, bool has_dest = true, bool has_args = true,
              size_t args_size = T::OP_SIZE>
    FuncPtr MakeInstruction(FuncPtr func, sjp::Json &instr) {

        auto instr_ptr = std::make_unique<T>();

        if constexpr (has_dest) {
            auto dest = instr.Get("dest")->Get<std::string>().value();
            if (operands.contains(dest)) {
                // dest already exists
                instr_ptr->SetOperand(operands[dest]);
            } else {
                // create new dest reg operand
                std::unique_ptr<OperandBase> dest_oprnd = nullptr;
                if (instr.Get("type")->type == sjp::JsonType::jobject) {
                    // dest_oprnd = MakePtrOperand(dest, instr);
                    dest_oprnd =
                        ParsePtrType(std::make_unique<PtrOperand>(dest), instr);
                } else {
                    std::string type =
                        instr.Get("type")->Get<std::string>().value();
                    dest_oprnd = std::make_unique<RegOperand>(
                        GetDataTypeFromStr(type), dest);
                }
                operands[dest] = dest_oprnd.get();
                instr_ptr->SetOperand(dest_oprnd.get());
                func->AddOperand(std::move(dest_oprnd));
            }
        }

        if (has_args || instr.Get("args").has_value()) {
            auto args = instr.Get("args").value();
            assert((!has_args || args.Size() == args_size) &&
                   "Invaid argument size");

            for (size_t i : std::views::iota(0UL, args.Size())) {
                auto arg = args.Get(i)->Get<std::string>().value();
                CheckSymbol(arg, func->GetName().c_str(), "MakeInstruction");
                instr_ptr->SetOperand(operands[arg]);
            }
        }

        APPEND_INSTR(func, instr_ptr);

        return func;
    }

    template <typename T, DataType type>
    FuncPtr BuildConstInstruction(FuncPtr func, sjp::Json &instr,
                                  std::string type_str) {
        // One single ImmedOperand represents every instance of that constant
        T value = GetJsonValue<T>(instr.Get("value").value());
        auto instr_ptr = std::make_unique<ConstInstruction>();

        auto dest = instr.Get("dest")->Get<std::string>().value();
        if (operands.contains(dest)) {
            // dest already exists
            instr_ptr->SetOperand(operands[dest]);
        } else {
            // create new dest reg operand
            auto dest_oprnd = std::make_unique<RegOperand>(type, dest);
            operands[dest] = dest_oprnd.get();
            instr_ptr->SetOperand(dest_oprnd.get());
            func->AddOperand(std::move(dest_oprnd));
        }

        std::string sym_name = "_$K_" + type_str + "_" + std::to_string(value);
        if (operands.contains(sym_name)) {
            // Const already exists
            instr_ptr->SetOperand(operands[sym_name]);
        } else {
            // Create new Immed Operand
            auto src_oprnd =
                std::make_unique<ImmedOperand<T>>(type, sym_name, value);
            operands[sym_name] = src_oprnd.get();
            instr_ptr->SetOperand(src_oprnd.get());
            func->AddOperand(std::move(src_oprnd));
        }

        APPEND_INSTR(func, instr_ptr);

        return func;
    }

    inline void CheckSymbol(std::string &arg, const char *fname,
                            const char *instr) {
        if (!operands.contains(arg)) {
            throw std::runtime_error(
                std::format("Error parsing {} in function {}. "
                            "Use of an undefined operand {} found.\n",
                            instr, fname, arg));
        }
    }

    template <typename T> T ParsePtrType(T t, sjp::Json &data) {
        auto jtype = data.Get("type").value();
        do {
            auto type = jtype.Get("ptr");
            if (type->type == sjp::JsonType::jobject) {
                t->AppendPtrChain(DataType::PTR);
            } else {
                t->AppendPtrChain(
                    GetDataTypeFromStr(type->Get<std::string>().value()));
                break;
            }
            jtype = type.value();
        } while (true);
        return t;
    }

    template <typename T> static inline T GetJsonValue(sjp::Json json) {
        if constexpr (std::is_same_v<ValType::INT, T>) {
            return static_cast<T>(json.Get<double>().value());
        } else {
            return json.Get<T>().value();
        }
    }
};
} // namespace sc
