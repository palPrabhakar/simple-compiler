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

    template <bool first = false>
    FuncPtr ParseInstructions(FuncPtr func, sjp::Json &instr) {
        // Check Opcode
        if constexpr (first) {
            // Add this if it's not required the subsequent passes
            // will remove redundant block. This ensures that there
            // is always a unique entry block
            func = MakeNewBlock(std::move(func), "__sc_entry__");
        }

        Opcode Opcode = Opcode::NOP;
        auto Opcode_str = instr.Get("op");
        if (Opcode_str == std::nullopt) {
            if (instr.Get("label") == std::nullopt) {
                throw std::runtime_error(
                    std::format("Error parsing function {}. "
                                "No valid instructions found.\n",
                                func->GetName()));
            }
            Opcode = Opcode::LABEL;
        } else {
            Opcode = GetOpcodeFromStr(Opcode_str->Get<std::string>().value());
        }

        // If a br or jmp instr is processed then skip all instr until a label
        // is found since it is not possible to get to these instructions.
        // Since it is not possible to jmp to unnamed locations in bril[??]
        if (check) {
            check = Opcode != Opcode::LABEL;
            if (check) {
                return func;
            }
        }

        switch (Opcode) {
        // Arithmetic Instructions
        case Opcode::ADD:
            return MakeInstruction<AddInstruction>(std::move(func), instr);
        case Opcode::MUL:
            return MakeInstruction<MulInstruction>(std::move(func), instr);
        case Opcode::SUB:
            return MakeInstruction<SubInstruction>(std::move(func), instr);
        case Opcode::DIV:
            return MakeInstruction<DivInstruction>(std::move(func), instr);
        // Comparison Instructions
        case Opcode::EQ:
            return MakeInstruction<EqInstruction>(std::move(func), instr);
        case Opcode::LT:
            return MakeInstruction<LtInstruction>(std::move(func), instr);
        case Opcode::GT:
            return MakeInstruction<GtInstruction>(std::move(func), instr);
        case Opcode::LE:
            return MakeInstruction<LeInstruction>(std::move(func), instr);
        case Opcode::GE:
            return MakeInstruction<GeInstruction>(std::move(func), instr);
        // Logic Instructions
        case Opcode::AND:
            return MakeInstruction<AndInstruction>(std::move(func), instr);
        case Opcode::OR:
            return MakeInstruction<OrInstruction>(std::move(func), instr);
        case Opcode::NOT:
            return MakeInstruction<NotInstruction>(std::move(func), instr);
        // Control Instructions
        case Opcode::JMP: {
            // End block
            check = true;
            return MakeJmpInstruction(std::move(func), instr);
        }
        case Opcode::BR: {
            // End block
            check = true;
            return MakeBranchInstruction(std::move(func), instr);
        }
        case Opcode::CALL:
            return MakeCallInstruction(std::move(func), instr);
        case Opcode::RET: {
            return MakeRetInstruction(std::move(func), instr);
        } break;
        // SSA Instructions
        case Opcode::SET:
            assert(false && "todo set\n");
        case Opcode::GET:
            assert(false && "todo get\n");
        // Memory Instructions
        case Opcode::ALLOC:
            return MakeInstruction<AllocInstruction>(std::move(func), instr);
        case Opcode::FREE:
            return MakeInstruction<FreeInstruction, false>(std::move(func),
                                                           instr);
        case Opcode::LOAD:
            return MakeInstruction<LoadInstruction>(std::move(func), instr);
        case Opcode::STORE:
            return MakeInstruction<StoreInstruction, false>(std::move(func),
                                                            instr);
        case Opcode::PTRADD:
            return MakeInstruction<PtraddInstruction>(std::move(func), instr);
        // Floating Instructions
        case Opcode::FADD:
            return MakeInstruction<FAddInstruction>(std::move(func), instr);
        case Opcode::FMUL:
            return MakeInstruction<FMulInstruction>(std::move(func), instr);
        case Opcode::FSUB:
            return MakeInstruction<FSubInstruction>(std::move(func), instr);
        case Opcode::FDIV:
            return MakeInstruction<FDivInstruction>(std::move(func), instr);
        // Floating Comparisons
        case Opcode::FEQ:
            return MakeInstruction<FEqInstruction>(std::move(func), instr);
        case Opcode::FLT:
            return MakeInstruction<FLtInstruction>(std::move(func), instr);
        case Opcode::FLE:
            return MakeInstruction<FLeInstruction>(std::move(func), instr);
        case Opcode::FGT:
            return MakeInstruction<FGtInstruction>(std::move(func), instr);
        case Opcode::FGE:
            return MakeInstruction<FGeInstruction>(std::move(func), instr);
        // Miscellaneous Instructions
        case Opcode::ID:
            return MakeInstruction<IdInstruction>(std::move(func), instr);
        case Opcode::CONST:
            return MakeConstInstruction(std::move(func), instr);
        case Opcode::PRINT:
            return MakeInstruction<PrintInstruction, false>(std::move(func),
                                                            instr);
        case Opcode::LABEL: {
            // Make new block
            auto lbl = instr.Get("label")->Get<std::string>().value();
            return MakeNewBlock(std::move(func), lbl);
        };
        case Opcode::NOP: {
            auto instr_ptr = std::make_unique<NopInstruction>();
            APPEND_INSTR(func, instr_ptr);
            return func;
        }
        default:
            assert(false && "Invalid Opcode\n");
        }

        return func;
    }

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
