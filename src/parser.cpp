#include "function.hpp"
#include "instructions.hpp"
#include "json.hpp"
#include "opcodes.hpp"
#include "operands.hpp"
#include "program.hpp"
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace sc {
using sym_tbl = std::unordered_map<std::string, OperandBase *>;
static OpCode opcode = OpCode::NOP;

static inline void CheckSymbol(sym_tbl &symbols, std::string &arg,
                               const char *fname, const char *instr) {
    if (!symbols.contains(arg)) {
        throw std::runtime_error(
            std::format("Error parsing {} in function {}. "
                        "Use of an undefined operand {} found.\n",
                        instr, fname, arg));
    }
}

template <typename T> static inline T GetJsonValue(sjp::Json json) {
    if constexpr (std::is_same_v<int, T>) {
        return static_cast<int>(json.Get<double>().value());
    } else {
        return json.Get<T>().value();
    }
}

static std::unique_ptr<PtrOperand> MakePtrOperand(std::string name,
                                                  sjp::Json &data) {
    auto operand = std::make_unique<PtrOperand>(name);
    auto jtype = data.Get("type").value();
    do {
        auto type = jtype.Get("ptr");
        if (type->type == sjp::JsonType::jobject) {
            operand->AppendPtrChain(DataType::PTR);
        } else {
            operand->AppendPtrChain(
                GetDataTypeFromStr(type->Get<std::string>().value()));
            break;
        }
        jtype = type.value();
    } while (true);
    return operand;
}

template <typename T, DataType type>
static std::unique_ptr<Function>
BuildConstInstruction(std::unique_ptr<Function> func, sjp::Json &instr,
                      std::string type_str, sym_tbl &tbl) {
    // One single ImmedOperand represents every instance of that constant
    T value = GetJsonValue<T>(instr.Get("value").value());
    auto instr_ptr = std::make_unique<ConstInstruction>();
    auto dest = instr.Get("dest")->Get<std::string>().value();
    if (tbl.contains(dest)) {
        // dest already exists
        instr_ptr->SetOperand(tbl[dest]);
    } else {
        // create new dest reg operand
        auto dest_oprnd = std::make_unique<RegOperand>(type, dest);
        tbl[dest] = dest_oprnd.get();
        instr_ptr->SetOperand(dest_oprnd.get());
        func->AddOperand(std::move(dest_oprnd));
    }
    std::string sym_name = "_$K_" + type_str + "_" + std::to_string(value);
    if (tbl.contains(sym_name)) {
        // Const already exists
        instr_ptr->SetOperand(tbl[sym_name]);
    } else {
        // Create new Immed Operand
        auto src_oprnd =
            std::make_unique<ImmedOperand<T>>(type, sym_name, value);
        tbl[sym_name] = src_oprnd.get();
        instr_ptr->SetOperand(src_oprnd.get());
        func->AddOperand(std::move(src_oprnd));
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

template <typename T, bool isDest = true>
std::unique_ptr<Function> MakeInstruction(std::unique_ptr<Function> func,
                                          sjp::Json &instr, sym_tbl &symbols) {

    auto args = instr.Get("args").value();
    auto instr_ptr = std::make_unique<T>();

    if constexpr (isDest) {
        auto dest = instr.Get("dest")->Get<std::string>().value();
        if (symbols.contains(dest)) {
            // dest already exists
            instr_ptr->SetOperand(symbols[dest]);
        } else {
            // create new dest reg operand
            std::unique_ptr<OperandBase> dest_oprnd = nullptr;
            if (instr.Get("type")->type == sjp::JsonType::jobject) {
                dest_oprnd = MakePtrOperand(dest, instr);
            } else {
                std::string type =
                    instr.Get("type")->Get<std::string>().value();
                dest_oprnd = std::make_unique<RegOperand>(
                    GetDataTypeFromStr(type), dest);
            }
            symbols[dest] = dest_oprnd.get();
            instr_ptr->SetOperand(dest_oprnd.get());
            func->AddOperand(std::move(dest_oprnd));
        }
    }

    for (size_t i : std::views::iota(0UL, args.Size())) {
        auto arg = args.Get(i)->Get<std::string>().value();
        CheckSymbol(symbols, arg, func->GetName().c_str(), "MakeInstruction");
        instr_ptr->SetOperand(symbols[arg]);
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

// Control Instructions
std::unique_ptr<Function> MakeJmpInstruction(std::unique_ptr<Function> func,
                                             sjp::Json &instr,
                                             sym_tbl &symbols) {
    auto labels = instr.Get("labels").value();
    assert(labels.Size() == 1 && "Label size != 1 in JmpInstruction\n");
    auto instr_ptr = std::make_unique<JmpInstruction>();
    auto lbl = labels.Get(0)->Get<std::string>().value();
    if (symbols.contains(lbl)) {
        instr_ptr->SetOperand(symbols[lbl]);
    } else {
        auto operand = std::make_unique<LabelOperand>(lbl);
        symbols[lbl] = operand.get();
        instr_ptr->SetOperand(operand.get());
        func->AddOperand(std::move(operand));
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

std::unique_ptr<Function> MakeBranchInstruction(std::unique_ptr<Function> func,
                                                sjp::Json &instr,
                                                sym_tbl &symbols) {
    auto labels = instr.Get("labels").value();
    assert(labels.Size() == 2 && "Label size != 2 in BranchInstruction\n");
    auto args = instr.Get("args").value();
    assert(args.Size() == 1 && "Argument size != 1 in BranchInstruction\n");
    auto instr_ptr = std::make_unique<BranchInstruction>();
    for (size_t i : std::views::iota(0UL, labels.Size())) {
        auto lbl = labels.Get(i)->Get<std::string>().value();
        if (symbols.contains(lbl)) {
            instr_ptr->SetOperand(symbols[lbl]);
        } else {
            auto operand = std::make_unique<LabelOperand>(lbl);
            symbols[lbl] = operand.get();
            instr_ptr->SetOperand(operand.get());
            func->AddOperand(std::move(operand));
        }
    }
    auto arg = args.Get(0)->Get<std::string>().value();
    CheckSymbol(symbols, arg, func->GetName().c_str(), "BranchInstruction");
    instr_ptr->SetOperand(symbols[arg]);
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

std::unique_ptr<Function> MakeCallInstruction(std::unique_ptr<Function> func,
                                              sjp::Json &instr,
                                              sym_tbl &symbols) {
    auto jfuncs = instr.Get("funcs").value();
    assert(jfuncs.Size() == 1 &&
           "More than one function specified in CallInstruction\n");

    bool has_dest = instr.Get("type").has_value();
    if (has_dest) {
        func = MakeInstruction<CallInstruction, true>(std::move(func), instr,
                                                      symbols);
    } else {
        func = MakeInstruction<CallInstruction, false>(std::move(func), instr,
                                                       symbols);
    }

    auto fptr = static_cast<CallInstruction *>(
        func->GetInstruction(func->GetInstructionSize() - 1));
    fptr->SetFuncName(jfuncs.Get(0)->Get<std::string>().value());
    fptr->SetRetVal(has_dest);
    return func;
}

std::unique_ptr<Function> MakeRetInstruction(std::unique_ptr<Function> func,
                                             sjp::Json &instr,
                                             sym_tbl &symbols) {
    auto instr_ptr = std::make_unique<RetInstruction>();
    if (instr.Get("args").has_value()) {
        auto args = instr.Get("args").value();
        assert(args.Size() == 1 && "More than one return value specified\n");
        auto arg = args.Get(0)->Get<std::string>().value();
        CheckSymbol(symbols, arg, func->GetName().c_str(), "RetInstruction");
        instr_ptr->SetOperand(symbols[arg]);
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

// Memory Instructions
std::unique_ptr<Function> MakeFreeInstruction(std::unique_ptr<Function> func,
                                              sjp::Json &instr,
                                              sym_tbl &symbols) {
    auto args = instr.Get("args").value();
    assert(args.Size() == 1 && "Invaid argument size in FreeInstruction\n");
    auto arg = args.Get(0)->Get<std::string>().value();
    CheckSymbol(symbols, arg, func->GetName().c_str(), "FreeInstruction");
    auto instr_ptr = std::make_unique<FreeInstruction>();
    instr_ptr->SetOperand(symbols[arg]);
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

std::unique_ptr<Function> MakeStoreInstruction(std::unique_ptr<Function> func,
                                               sjp::Json &instr,
                                               sym_tbl &symbols) {
    auto args = instr.Get("args");
    assert(args->Size() == 2 && "Invalid args size in StoreInstruction\n");
    auto instr_ptr = std::make_unique<StoreInstruction>();
    for (size_t i : std::views::iota(0UL, args->Size())) {
        auto arg = args->Get(i)->Get<std::string>().value();
        CheckSymbol(symbols, arg, func->GetName().c_str(), "StoreInstruction");
        instr_ptr->SetOperand(symbols[arg]);
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

// Miscellaneous Instructions
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

std::unique_ptr<Function> MakePrintInstruction(std::unique_ptr<Function> func,
                                               sjp::Json &instr,
                                               sym_tbl &symbols) {
    auto args = instr.Get("args").value();
    if (args.Size() == 0)
        // Nothing to print
        return func;

    auto instr_ptr = std::make_unique<PrintInstruction>();
    for (size_t i : std::views::iota(0UL, args.Size())) {
        auto arg = args.Get(i)->Get<std::string>().value();
        CheckSymbol(symbols, arg, func->GetName().c_str(), "PrintInstruction");
        instr_ptr->SetOperand(symbols[arg]);
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

std::unique_ptr<Function> MakeLabelInstruction(std::unique_ptr<Function> func,
                                               sjp::Json &instr,
                                               sym_tbl &symbols) {
    auto lbl = instr.Get("label")->Get<std::string>().value();
    auto instr_ptr = std::make_unique<LabelInstruction>();
    if (symbols.contains(lbl)) {
        instr_ptr->SetOperand(symbols[lbl]);
    } else {
        auto operand = std::make_unique<LabelOperand>(lbl);
        symbols[lbl] = operand.get();
        instr_ptr->SetOperand(operand.get());
        func->AddOperand(std::move(operand));
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

// Parse Functions
std::unique_ptr<Function> ParseInstructions(std::unique_ptr<Function> func,
                                            sjp::Json &instr,
                                            sym_tbl &symbols) {
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
    switch (opcode) {
    // Arithmetic Instructions
    case OpCode::ADD:
        return MakeInstruction<AddInstruction>(std::move(func), instr, symbols);
    case OpCode::MUL:
        return MakeInstruction<MulInstruction>(std::move(func), instr, symbols);
    case OpCode::SUB:
        return MakeInstruction<SubInstruction>(std::move(func), instr, symbols);
    case OpCode::DIV:
        return MakeInstruction<DivInstruction>(std::move(func), instr, symbols);
    // Comparison Instructions
    case OpCode::EQ:
        return MakeInstruction<EqInstruction>(std::move(func), instr, symbols);
    case OpCode::LT:
        return MakeInstruction<LtInstruction>(std::move(func), instr, symbols);
    case OpCode::GT:
        return MakeInstruction<GtInstruction>(std::move(func), instr, symbols);
    case OpCode::LE:
        return MakeInstruction<LeInstruction>(std::move(func), instr, symbols);
    case OpCode::GE:
        return MakeInstruction<GeInstruction>(std::move(func), instr, symbols);
    // Logic Instructions
    case OpCode::AND:
        return MakeInstruction<AndInstruction>(std::move(func), instr, symbols);
    case OpCode::OR:
        return MakeInstruction<OrInstruction>(std::move(func), instr, symbols);
    case OpCode::NOT:
        return MakeInstruction<NotInstruction>(std::move(func), instr, symbols);
    // Control Instructions
    case OpCode::JMP:
        return MakeJmpInstruction(std::move(func), instr, symbols);
        break;
    case OpCode::BR:
        return MakeBranchInstruction(std::move(func), instr, symbols);
    case OpCode::CALL:
        return MakeCallInstruction(std::move(func), instr, symbols);
    case OpCode::RET:
        return MakeRetInstruction(std::move(func), instr, symbols);
    // SSA Instructions
    case OpCode::SET:
        assert(false && "todo set\n");
    case OpCode::GET:
        assert(false && "todo get\n");
    // Memory Instructions
    case OpCode::ALLOC:
        return MakeInstruction<AllocInstruction>(std::move(func), instr,
                                                 symbols);
    case OpCode::FREE:
        return MakeFreeInstruction(std::move(func), instr, symbols);
    case OpCode::LOAD:
        return MakeInstruction<LoadInstruction>(std::move(func), instr,
                                                symbols);
    case OpCode::STORE:
        return MakeStoreInstruction(std::move(func), instr, symbols);
    case OpCode::PTRADD:
        return MakeInstruction<PtraddInstruction>(std::move(func), instr,
                                                  symbols);
    // Floating Instructions
    case OpCode::FADD:
        return MakeInstruction<FAddInstruction>(std::move(func), instr,
                                                symbols);
    case OpCode::FMUL:
        return MakeInstruction<FMulInstruction>(std::move(func), instr,
                                                symbols);
    case OpCode::FSUB:
        return MakeInstruction<FSubInstruction>(std::move(func), instr,
                                                symbols);
    case OpCode::FDIV:
        return MakeInstruction<FDivInstruction>(std::move(func), instr,
                                                symbols);
    // Floating Comparisons
    case OpCode::FEQ:
        return MakeInstruction<FEqInstruction>(std::move(func), instr, symbols);
    case OpCode::FLT:
        return MakeInstruction<FLtInstruction>(std::move(func), instr, symbols);
    case OpCode::FLE:
        return MakeInstruction<FLeInstruction>(std::move(func), instr, symbols);
    case OpCode::FGT:
        return MakeInstruction<FGtInstruction>(std::move(func), instr, symbols);
    case OpCode::FGE:
        return MakeInstruction<FGeInstruction>(std::move(func), instr, symbols);
    // Miscellaneous Instructions
    case OpCode::ID:
        return MakeInstruction<IdInstruction>(std::move(func), instr, symbols);
    case OpCode::CONST:
        return MakeConstInstruction(std::move(func), instr, symbols);
    case OpCode::PRINT:
        return MakePrintInstruction(std::move(func), instr, symbols);
    case OpCode::LABEL:
        return MakeLabelInstruction(std::move(func), instr, symbols);
    case OpCode::NOP:
        assert(false && "todo nop\n");
    default:
        assert(false && "Invalid opcode\n");
    }
    return nullptr;
}

std::unique_ptr<Function> ParseBody(std::unique_ptr<Function> func,
                                    sjp::Json &instrs, sym_tbl &symbols) {
    for (size_t i : std::views::iota(0UL, instrs.Size())) {
        auto instr = instrs.Get(i).value();
        func = ParseInstructions(std::move(func), instr, symbols);
    }
    return func;
}

std::unique_ptr<Function> ParseArguments(std::unique_ptr<Function> func,
                                         sjp::Json &args, sym_tbl &symbols) {
    // TODO:
    // Set register number
    for (size_t i : std::views::iota(0UL, args.Size())) {
        auto jop = args.Get(i).value();
        std::string name = jop.Get("name")->Get<std::string>().value();
        if (symbols.contains(name)) {
            throw std::runtime_error(
                std::format("Error parsing arguments of function {}. "
                            "Redeclaration of argument name {}\n",
                            func->GetName(), name));
        }

        std::unique_ptr<OperandBase> operand = nullptr;
        if (jop.Get("type")->type == sjp::JsonType::jobject) {
            operand = MakePtrOperand(name, jop);
        } else {
            std::string type = jop.Get("type")->Get<std::string>().value();
            operand =
                std::make_unique<RegOperand>(GetDataTypeFromStr(type), name);
        }
        func->AddArgs(operand.get());
        symbols[name] = operand.get();
        func->AddOperand(std::move(operand));
    }
    return func;
}

std::unique_ptr<Function> ParseFunction(sjp::Json jfunc) {
    sym_tbl symbols;
    std::unique_ptr<Function> func = nullptr;
    auto fn_name = jfunc.Get("name")->Get<std::string>().value();
    if (jfunc.Get("type").has_value()) {
        if (jfunc.Get("type")->type == sjp::JsonType::jobject) {
            func = std::make_unique<PtrFunction>(fn_name);
            auto jtype = jfunc.Get("type").value();
            do {
                auto type = jtype.Get("ptr");
                if (type->type == sjp::JsonType::jobject) {
                    static_cast<PtrFunction *>(func.get())
                        ->AppendPtrChain(DataType::PTR);
                } else {
                    static_cast<PtrFunction *>(func.get())
                        ->AppendPtrChain(GetDataTypeFromStr(
                            type->Get<std::string>().value()));
                    break;
                }
                jtype = type.value();
            } while (true);
        } else {
            auto type = GetDataTypeFromStr(
                jfunc.Get("type")->Get<std::string>().value());
            func = std::make_unique<Function>(fn_name, type);
        }
    } else {
        func = std::make_unique<Function>(fn_name, DataType::VOID);
    }
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
    auto functions = jprogram.Get("functions");
    for (size_t i : std::views::iota(0UL, functions->Size())) {
        program->AddFunction(ParseFunction(functions->Get(i).value()));
    }
    return program;
}
} // namespace sc
