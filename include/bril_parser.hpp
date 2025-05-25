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

    FuncPtr MakeNewBlock(FuncPtr func, std::string name);
    FuncPtr MakeJmpInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr MakeBranchInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr MakeCallInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr MakeRetInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr MakeConstInstruction(FuncPtr func, sjp::Json &instr);
    FuncPtr ParseInstructions(FuncPtr func, sjp::Json &instr);

    template <typename T, bool has_dest = true>
    FuncPtr MakeInstruction(FuncPtr func, sjp::Json &instr) {

        auto instr_ptr = std::make_unique<T>();

        if constexpr (has_dest) {
            auto dest = instr.Get("dest")->Get<std::string>().value();
            if (operands.contains(dest)) {
                // dest already exists
                instr_ptr->SetDest(operands[dest]);
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
                instr_ptr->SetDest(dest_oprnd.get());
                func->AddOperand(std::move(dest_oprnd));
            }
        }

        if (instr.Get("args").has_value()) {
            auto args = instr.Get("args").value();
            for (size_t i : std::views::iota(0UL, args.Size())) {
                auto arg = args.Get(i)->Get<std::string>().value();
                // It is possible that the block that defines the args
                // comes later in text but happens before in control flow
                assert(operands.contains(arg) &&
                       "todo: add support of parsing args before def\n");
                instr_ptr->SetOperand(operands[arg]);
            }
        }

        APPEND_INSTR(func, instr_ptr);

        return func;
    }

    template <typename T, DataType type>
    FuncPtr BuildConstInstruction(FuncPtr func, sjp::Json &instr) {
        // One single ImmedOperand represents every instance of that constant
        typename T::val_type value =
            GetJsonValue<typename T::val_type>(instr.Get("value").value());

        auto instr_ptr = std::make_unique<ConstInstruction>();

        auto dest = instr.Get("dest")->Get<std::string>().value();
        if (operands.contains(dest)) {
            // dest already exists
            instr_ptr->SetDest(operands[dest]);
        } else {
            // create new dest reg operand
            auto dest_oprnd = std::make_unique<RegOperand>(type, dest);
            operands[dest] = dest_oprnd.get();
            instr_ptr->SetDest(dest_oprnd.get());
            func->AddOperand(std::move(dest_oprnd));
        }

        // ImmedOperands are not own by function
        // They are managed in a separately. This
        // ensures there is always one ImmedOperand
        // per <data_type, value> pair.
        instr_ptr->SetOperand(T::GetOperand(value));

        APPEND_INSTR(func, instr_ptr);

        return func;
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
