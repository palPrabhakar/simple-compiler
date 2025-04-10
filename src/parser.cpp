#include "function.hpp"
#include "instructions.hpp"
#include "json.hpp"
#include "opcodes.hpp"
#include "operands.hpp"
#include "program.hpp"
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace sc {
using sym_tbl = std::unordered_map<std::string, OperandBase *>;
static OpCode opcode = OpCode::NOP;

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
        auto operand =
            std::make_unique<RegOperand>(GetDataTypeFromStr(type), name);
        func->AddArgs(operand.get());
        symbols[name] = operand.get();
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
    auto instr_ptr = std::make_unique<ConstInstruction>();
    auto dest = instr.Get("dest")->Get<std::string>().value();
    if (tbl.contains(dest)) {
        // dest already exists
        instr_ptr->SetOperand(tbl[dest], 0);
    } else {
        // create new dest reg operand
        auto dest_oprnd = std::make_unique<RegOperand>(type, dest);
        tbl[dest] = dest_oprnd.get();
        instr_ptr->SetOperand(dest_oprnd.get(), 0);
        func->AddOperand(std::move(dest_oprnd));
    }
    std::string sym_name = "_$K_" + type_str + "_" + std::to_string(value);
    if (tbl.contains(sym_name)) {
        // Const already exists
        instr_ptr->SetOperand(tbl[sym_name], 1);
    } else {
        // Create new Immed Operand
        auto src_oprnd =
            std::make_unique<ImmedOperand<T>>(type, sym_name, value);
        tbl[sym_name] = src_oprnd.get();
        instr_ptr->SetOperand(src_oprnd.get(), 1);
        func->AddOperand(std::move(src_oprnd));
    }
    func->AddInstructions(std::move(instr_ptr));
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

template <typename T, size_t argSize = 2>
std::unique_ptr<Function> MakeInstruction(std::unique_ptr<Function> func,
                                          sjp::Json &instr, sym_tbl &symbols) {

    auto args = instr.Get("args").value();
    assert(args.Size() == argSize &&
           "Arithmetic operator size not equal to 2\n");

    auto instr_ptr = std::make_unique<T>();
    auto dest = instr.Get("dest")->Get<std::string>().value();
    auto type_str = instr.Get("type")->Get<std::string>().value();
    DataType type = GetDataTypeFromStr(type_str);
    if (symbols.contains(dest)) {
        // dest already exists
        instr_ptr->SetOperand(symbols[dest], 0);
    } else {
        // create new dest reg operand
        auto dest_oprnd = std::make_unique<RegOperand>(type, dest);
        symbols[dest] = dest_oprnd.get();
        instr_ptr->SetOperand(dest_oprnd.get(), 0);
        func->AddOperand(std::move(dest_oprnd));
    }
    for (size_t i = 0; i < args.Size(); ++i) {
        auto arg = args.Get(i)->Get<std::string>().value();
        if (!symbols.contains(arg)) {
            throw std::runtime_error(std::format(
                "Error parsing ArithmeticInstruction in function {}."
                "Use of an undefined operand {} found.\n",
                func->GetName(), arg));
        }
        instr_ptr->SetOperand(symbols[arg], i + 1);
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

std::unique_ptr<Function> MakeJmpInstruction(std::unique_ptr<Function> func,
                                             sjp::Json &instr,
                                             sym_tbl &symbols) {
    auto labels = instr.Get("labels").value();
    assert(labels.Size() == 1 && "Label size != 1 in JmpInstruction\n");
    auto instr_ptr = std::make_unique<BranchInstruction>();
    auto lbl = labels.Get(0)->Get<std::string>().value();
    if (!symbols.contains(lbl)) {
        auto operand = std::make_unique<LabelOperand>(lbl);
        symbols[lbl] = operand.get();
        instr_ptr->SetOperand(operand.get(), 0);
        func->AddOperand(std::move(operand));
    } else {
        instr_ptr->SetOperand(symbols[lbl], 0);
    }
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

    for (size_t i = 0; i < labels.Size(); ++i) {
        auto lbl = labels.Get(i)->Get<std::string>().value();
        if (!symbols.contains(lbl)) {
            auto operand = std::make_unique<LabelOperand>(lbl);
            symbols[lbl] = operand.get();
            instr_ptr->SetOperand(operand.get(), i);
            func->AddOperand(std::move(operand));
        } else {
            instr_ptr->SetOperand(symbols[lbl], i);
        }
    }

    auto arg = args.Get(0)->Get<std::string>().value();
    if (!symbols.contains(arg)) {
        throw std::runtime_error(
            std::format("Error parsing BranchInstruction in function {}."
                        "Use of an undefined operand {} found.\n",
                        func->GetName(), arg));
    }
    instr_ptr->SetOperand(symbols[arg], 2);
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

std::unique_ptr<Function> MakeCallInstruction(std::unique_ptr<Function> func,
                                              sjp::Json &instr,
                                              sym_tbl &symbols) {
    auto jfuncs = instr.Get("funcs").value();
    assert(jfuncs.Size() == 1 &&
           "More than one function specified in CallInstruction\n");
    auto instr_ptr = std::make_unique<CallInstruction>();
    instr_ptr->SetFuncName(jfuncs.Get(0)->Get<std::string>().value());

    // first do dest, since first argument
    auto dest = instr.Get("dest")->Get<std::string>().value();
    if (symbols.contains(dest)) {
        // dest already exists
        instr_ptr->SetOperand(symbols[dest], 0);
    } else {
        // create new dest reg operand
        DataType type =
            GetDataTypeFromStr(instr.Get("type")->Get<std::string>().value());
        auto dest_oprnd = std::make_unique<RegOperand>(type, dest);
        symbols[dest] = dest_oprnd.get();
        instr_ptr->SetOperand(dest_oprnd.get(), 0);
        func->AddOperand(std::move(dest_oprnd));
    }

    auto args = instr.Get("args").value();
    for (size_t i = 0; i < args.Size(); ++i) {
        auto arg = args.Get(i)->Get<std::string>().value();
        if (!symbols.contains(arg)) {
            throw std::runtime_error(
                std::format("Error parsing CallInstruction in function {}."
                            "Use of an undefined operand {} found.\n",
                            func->GetName(), arg));
        }
        instr_ptr->SetOperand(symbols[arg], i + 1);
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

std::unique_ptr<Function> MakeRetInstruction(std::unique_ptr<Function> func,
                                             sjp::Json &instr,
                                             sym_tbl &symbols) {
    auto args = instr.Get("args").value();
    assert(args.Size() == 1 && "More than one return value specified\n");
    auto instr_ptr = std::make_unique<RetInstruction>();
    auto arg = args.Get(0)->Get<std::string>().value();
    if (!symbols.contains(arg)) {
        throw std::runtime_error(
            std::format("Error parsing RetInstruction in function {}."
                        "Use of an undefined return-value {} found.\n",
                        func->GetName(), arg));
    }
    instr_ptr->SetOperand(symbols[arg], 0);
    func->AddInstructions(std::move(instr_ptr));
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
    for (size_t i = 0; i < args.Size(); ++i) {
        auto arg = args.Get(i)->Get<std::string>().value();
        if (!symbols.contains(arg)) {
            throw std::runtime_error(
                std::format("Error parsing PrintInstruction in function {}."
                            "Use of an undefined operand {} found.\n",
                            func->GetName(), arg));
        }
        instr_ptr->SetOperand(symbols[arg], i);
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

std::unique_ptr<Function> MakeLabelInstruction(std::unique_ptr<Function> func,
                                               sjp::Json &instr,
                                               sym_tbl &symbols) {
    auto lbl = instr.Get("label")->Get<std::string>().value();
    auto instr_ptr = std::make_unique<LabelInstruction>();
    if (!symbols.contains(lbl)) {
        auto operand = std::make_unique<LabelOperand>(lbl);
        symbols[lbl] = operand.get();
        instr_ptr->SetOperand(operand.get(), 0);
        func->AddOperand(std::move(operand));
    } else {
        instr_ptr->SetOperand(symbols[lbl], 0);
    }
    func->AddInstructions(std::move(instr_ptr));
    return func;
}

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
        return MakeInstruction<GeInstruction>(std::move(func), instr, symbols);
    case OpCode::OR:
        return MakeInstruction<GeInstruction>(std::move(func), instr, symbols);
    case OpCode::NOT:
        return MakeInstruction<NotInstruction, 1>(std::move(func), instr,
                                                  symbols);
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
        assert(false && "todo alloc\n");
    case OpCode::FREE:
        assert(false && "todo free\n");
    case OpCode::LOAD:
        assert(false && "todo load\n");
    case OpCode::STORE:
        assert(false && "todo store\n");
    case OpCode::PTRADD:
        assert(false && "todo ptradd\n");
    // Floating Instructions
    case OpCode::FADD:
        assert(false && "todo fadd\n");
    case OpCode::FMUL:
        assert(false && "todo fmul");
    case OpCode::FSUB:
        assert(false && "todo fsub\n");
    case OpCode::FDIV:
        assert(false && "todo fdiv\n");
    // Floating Comparisons
    case OpCode::FEQ:
        assert(false && "todo feq\n");
    case OpCode::FLT:
        assert(false && "todo flt");
    case OpCode::FLE:
        assert(false && "todo fle\n");
    case OpCode::FGT:
        assert(false && "todo fgt\n");
    case OpCode::FGE:
        assert(false && "todo fge\n");
    // Miscellaneous Instructions
    case OpCode::ID:
        return MakeInstruction<IdInstruction, 1>(std::move(func), instr,
                                                 symbols);
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
