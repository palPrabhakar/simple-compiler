#include "bril_parser.hpp"
#include "block.hpp"
#include "function.hpp"
#include "instruction.hpp"
#include "json.hpp"
#include "opcodes.hpp"
#include "operand.hpp"
#include "program.hpp"
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>

namespace sc {
FuncPtr BrilParser::MakeNewBlock(FuncPtr func, std::string name) {
    // add the label to the block
    // add the block to the label
    func->AddBlock(std::make_unique<Block>(name));

    // Check if label already exists
    if (labels.contains(name)) {
        // label for this block already exist
        LAST_BLK(func)->SetLabel(labels[name]);
        LAST_BLK(func)->GetLabel()->SetBlock(LAST_BLK(func));
    } else {
        auto operand = std::make_unique<LabelOperand>(name);
        labels[name] = operand.get();
        LAST_BLK(func)->SetLabel(operand.get());
        operand->SetBlock(LAST_BLK(func));
        func->AddOperand(std::move(operand));
    }

    return func;
}

// Control Instructions
FuncPtr BrilParser::MakeJmpInstruction(FuncPtr func, sjp::Json &instr) {
    auto jlbl = instr.Get("labels").value();
    assert(jlbl.Size() == 1 && "Label size != 1 in JmpInstruction\n");

    auto instr_ptr = std::make_unique<JmpInstruction>();

    // Check if label already exists
    auto lbl = jlbl.Get(0)->Get<std::string>().value();
    if (labels.contains(lbl)) {
        instr_ptr->SetOperand(labels[lbl]);
    } else {
        auto operand = std::make_unique<LabelOperand>(lbl);
        labels[lbl] = operand.get();
        instr_ptr->SetOperand(operand.get());
        func->AddOperand(std::move(operand));
    }

    APPEND_INSTR(func, instr_ptr);

    return func;
}

FuncPtr BrilParser::MakeBranchInstruction(FuncPtr func, sjp::Json &instr) {
    auto jlbl = instr.Get("labels").value();
    assert(jlbl.Size() == 2 && "Label size != 2 in BranchInstruction\n");

    auto args = instr.Get("args").value();
    assert(args.Size() == 1 && "Argument size != 1 in BranchInstruction\n");

    auto instr_ptr = std::make_unique<BranchInstruction>();

    // Check branch targets
    for (size_t i : std::views::iota(0UL, jlbl.Size())) {
        auto lbl = jlbl.Get(i)->Get<std::string>().value();
        if (labels.contains(lbl)) {
            instr_ptr->SetOperand(labels[lbl]);
        } else {
            auto operand = std::make_unique<LabelOperand>(lbl);
            labels[lbl] = operand.get();
            instr_ptr->SetOperand(operand.get());
            func->AddOperand(std::move(operand));
        }
    }

    // Check conditional value
    auto arg = args.Get(0)->Get<std::string>().value();
    CheckSymbol(arg, func->GetName().c_str(), "BranchInstruction");
    instr_ptr->SetOperand(operands[arg]);

    APPEND_INSTR(func, instr_ptr);

    return func;
}

FuncPtr BrilParser::MakeCallInstruction(FuncPtr func, sjp::Json &instr) {
    auto jfuncs = instr.Get("funcs").value();
    assert(jfuncs.Size() == 1 &&
           "More than one function specified in CallInstruction\n");

    bool has_dest = instr.Get("type").has_value();
    if (has_dest) {
        func = MakeInstruction<CallInstruction, true, false>(std::move(func),
                                                             instr);
    } else {
        func = MakeInstruction<CallInstruction, false, false>(std::move(func),
                                                              instr);
    }

    auto block = LAST_BLK(func);
    auto iptr = static_cast<CallInstruction *>(LAST_INSTR(block));
    iptr->SetFuncName(jfuncs.Get(0)->Get<std::string>().value());
    iptr->SetRetVal(has_dest);

    return func;
}

// Miscellaneous Instructions
FuncPtr BrilParser::MakeConstInstruction(FuncPtr func, sjp::Json &instr) {
    auto type_str = instr.Get("type")->Get<std::string>().value();
    DataType type = GetDataTypeFromStr(type_str);
    switch (type) {
    case DataType::INT:
        return BuildConstInstruction<ValType::INT, DataType::INT>(
            std::move(func), instr, std::move(type_str));
    case DataType::FLOAT:
        return BuildConstInstruction<ValType::FLOAT, DataType::FLOAT>(
            std::move(func), instr, std::move(type_str));
    case DataType::BOOL:
        return BuildConstInstruction<ValType::BOOL, DataType::BOOL>(
            std::move(func), instr, std::move(type_str));
    default:
        assert(false && "Unexpected type found in MakeConstInstruction\n");
        break;
    }
    return func;
}

FuncPtr BrilParser::MakePrintInstruction(FuncPtr func, sjp::Json &instr) {
    auto args = instr.Get("args").value();

    // Nothing to print
    if (args.Size() == 0)
        return func;

    return MakeInstruction<PrintInstruction, false, false>(std::move(func),
                                                           instr);
}

// Parse Functions
FuncPtr BrilParser::ParseInstructions(FuncPtr func, sjp::Json &instr) {
    // Check opcode
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
        return MakeInstruction<RetInstruction, false, false>(std::move(func),
                                                             instr);
    // SSA Instructions
    case OpCode::SET:
        assert(false && "todo set\n");
    case OpCode::GET:
        assert(false && "todo get\n");
    // Memory Instructions
    case OpCode::ALLOC:
        return MakeInstruction<AllocInstruction>(std::move(func), instr);
    case OpCode::FREE:
        return MakeInstruction<FreeInstruction, false, true>(std::move(func),
                                                             instr);
    case OpCode::LOAD:
        return MakeInstruction<LoadInstruction>(std::move(func), instr);
    case OpCode::STORE:
        return MakeInstruction<StoreInstruction, false>(std::move(func), instr);
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
        return MakePrintInstruction(std::move(func), instr);
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

std::unique_ptr<Function> BrilParser::ParseBody(std::unique_ptr<Function> func,
                                                sjp::Json &instrs) {
    check = false;
    func = MakeNewBlock(std::move(func), "entry");
    for (size_t i : std::views::iota(0UL, instrs.Size())) {
        auto instr = instrs.Get(i).value();
        func = ParseInstructions(std::move(func), instr);
    }
    return func;
}

FuncPtr BrilParser::ParseArguments(FuncPtr func, sjp::Json &args) {
    // TODO:
    // Set register number
    for (size_t i : std::views::iota(0UL, args.Size())) {
        auto jop = args.Get(i).value();
        std::string name = jop.Get("name")->Get<std::string>().value();
        if (operands.contains(name)) {
            throw std::runtime_error(
                std::format("Error parsing arguments of function {}. "
                            "Redeclaration of argument name {}\n",
                            func->GetName(), name));
        }

        std::unique_ptr<OperandBase> operand = nullptr;
        if (jop.Get("type")->type == sjp::JsonType::jobject) {
            operand = ParsePtrType(std::make_unique<PtrOperand>(name), jop);
        } else {
            std::string type = jop.Get("type")->Get<std::string>().value();
            operand =
                std::make_unique<RegOperand>(GetDataTypeFromStr(type), name);
        }

        func->AddArgs(operand.get());
        operands[name] = operand.get();
        func->AddOperand(std::move(operand));
    }
    return func;
}

std::unique_ptr<Function> BrilParser::ParseFunction(sjp::Json &jfunc) {
    operands.clear();
    labels.clear();

    std::unique_ptr<Function> func = nullptr;
    auto fn_name = jfunc.Get("name")->Get<std::string>().value();

    // Check if function has a return type
    if (jfunc.Get("type").has_value()) {
        if (jfunc.Get("type")->type == sjp::JsonType::jobject) {
            func = ParsePtrType(std::make_unique<PtrFunction>(fn_name), jfunc);
        } else {
            auto type = GetDataTypeFromStr(
                jfunc.Get("type")->Get<std::string>().value());
            func = std::make_unique<Function>(fn_name, type);
        }
    } else {
        func = std::make_unique<Function>(fn_name, DataType::VOID);
    }

    // If function takes arguments parse them
    auto args = jfunc.Get("args");
    if (jfunc.Get("args").has_value()) {
        func = ParseArguments(std::move(func), args.value());
    }

    // Parse function body
    auto instrs = jfunc.Get("instrs");
    func = ParseBody(std::move(func), instrs.value());
    return func;
}

std::unique_ptr<Program> BrilParser::ParseProgram() {
    auto program = std::make_unique<Program>();
    auto functions = data.Get("functions");
    for (size_t i : std::views::iota(0UL, functions->Size())) {
        auto jfunc = functions->Get(i).value();
        program->AddFunction(ParseFunction(jfunc));
    }
    return program;
}
} // namespace sc
