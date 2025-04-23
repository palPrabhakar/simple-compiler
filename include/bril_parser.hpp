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
    FuncPtr MakeConstInstruction(FuncPtr func, sjp::Json &instr);

    template <bool first = false>
    FuncPtr ParseInstructions(FuncPtr func, sjp::Json &instr) {
        // Check opcode
        if constexpr (first) {
            // Add this if it's not required the subsequent passes
            // will remove redundant block. This ensures that there
            // is always a unique entry block
            func = MakeNewBlock(std::move(func), "__sc_entry__");
        }

        OpCode opcode = OpCode::NOP;
        auto opcode_str = instr.Get("op");
        if (opcode_str == std::nullopt) {
            if (instr.Get("label") == std::nullopt) {
                throw std::runtime_error(
                    std::format("Error parsing function {}. "
                                "No valid instructions found.\n",
                                func->GetName()));
            }
            opcode = OpCode::LABEL;
        } else {
            opcode = GetOpCodeFromStr(opcode_str->Get<std::string>().value());
        }

        // If a br or jmp instr is processed then skip all instr until a label
        // is found since it is not possible to get to these instructions.
        // Since it is not possible to jmp to unnamed locations in bril[??]
        if (check) {
            check = opcode != OpCode::LABEL;
            if (check) {
                return func;
            }
        }

        switch (opcode) {
        // Arithmetic Instructions
        case OpCode::ADD:
            return MakeInstruction<AddInstruction>(std::move(func), instr);
        case OpCode::MUL:
            return MakeInstruction<MulInstruction>(std::move(func), instr);
        case OpCode::SUB:
            return MakeInstruction<SubInstruction>(std::move(func), instr);
        case OpCode::DIV:
            return MakeInstruction<DivInstruction>(std::move(func), instr);
        // Comparison Instructions
        case OpCode::EQ:
            return MakeInstruction<EqInstruction>(std::move(func), instr);
        case OpCode::LT:
            return MakeInstruction<LtInstruction>(std::move(func), instr);
        case OpCode::GT:
            return MakeInstruction<GtInstruction>(std::move(func), instr);
        case OpCode::LE:
            return MakeInstruction<LeInstruction>(std::move(func), instr);
        case OpCode::GE:
            return MakeInstruction<GeInstruction>(std::move(func), instr);
        // Logic Instructions
        case OpCode::AND:
            return MakeInstruction<AndInstruction>(std::move(func), instr);
        case OpCode::OR:
            return MakeInstruction<OrInstruction>(std::move(func), instr);
        case OpCode::NOT:
            return MakeInstruction<NotInstruction>(std::move(func), instr);
        // Control Instructions
        case OpCode::JMP: {
            // End block
            check = true;
            return MakeJmpInstruction(std::move(func), instr);
        }
        case OpCode::BR: {
            // End block
            check = true;
            return MakeBranchInstruction(std::move(func), instr);
        }
        case OpCode::CALL:
            return MakeCallInstruction(std::move(func), instr);
        case OpCode::RET:
            return MakeInstruction<RetInstruction, false, false>(
                std::move(func), instr);
        // SSA Instructions
        case OpCode::SET:
            assert(false && "todo set\n");
        case OpCode::GET:
            assert(false && "todo get\n");
        // Memory Instructions
        case OpCode::ALLOC:
            return MakeInstruction<AllocInstruction>(std::move(func), instr);
        case OpCode::FREE:
            return MakeInstruction<FreeInstruction, false, true>(
                std::move(func), instr);
        case OpCode::LOAD:
            return MakeInstruction<LoadInstruction>(std::move(func), instr);
        case OpCode::STORE:
            return MakeInstruction<StoreInstruction, false>(std::move(func),
                                                            instr);
        case OpCode::PTRADD:
            return MakeInstruction<PtraddInstruction>(std::move(func), instr);
        // Floating Instructions
        case OpCode::FADD:
            return MakeInstruction<FAddInstruction>(std::move(func), instr);
        case OpCode::FMUL:
            return MakeInstruction<FMulInstruction>(std::move(func), instr);
        case OpCode::FSUB:
            return MakeInstruction<FSubInstruction>(std::move(func), instr);
        case OpCode::FDIV:
            return MakeInstruction<FDivInstruction>(std::move(func), instr);
        // Floating Comparisons
        case OpCode::FEQ:
            return MakeInstruction<FEqInstruction>(std::move(func), instr);
        case OpCode::FLT:
            return MakeInstruction<FLtInstruction>(std::move(func), instr);
        case OpCode::FLE:
            return MakeInstruction<FLeInstruction>(std::move(func), instr);
        case OpCode::FGT:
            return MakeInstruction<FGtInstruction>(std::move(func), instr);
        case OpCode::FGE:
            return MakeInstruction<FGeInstruction>(std::move(func), instr);
        // Miscellaneous Instructions
        case OpCode::ID:
            return MakeInstruction<IdInstruction>(std::move(func), instr);
        case OpCode::CONST:
            return MakeConstInstruction(std::move(func), instr);
        case OpCode::PRINT:
            return MakeInstruction<PrintInstruction, false, false>(
                std::move(func), instr);
        case OpCode::LABEL: {
            // Make new block
            auto lbl = instr.Get("label")->Get<std::string>().value();
            return MakeNewBlock(std::move(func), lbl);
        };
        case OpCode::NOP: {
            auto instr_ptr = std::make_unique<NopInstruction>();
            APPEND_INSTR(func, instr_ptr);
            return func;
        }
        default:
            assert(false && "Invalid opcode\n");
        }

        return func;
    }

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
