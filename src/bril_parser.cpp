#include "bril_parser.hpp"
#include "block.hpp"
#include "function.hpp"
#include "instruction.hpp"
#include "parser.hpp"
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
    // It is possible for a label to show up
    // before the actual block like in a jmp
    // or br instruction
    if (labels.contains(name)) {
        // label for this block already exist
        LAST_BLK(func)->SetLabel(std::move(lbl_store[name]));
        LAST_BLK(func)->GetLabel()->SetBlock(LAST_BLK(func));
    } else {
        auto operand = std::make_unique<LabelOperand>(name, func->ctx.get_next_id());
        labels[name] = operand.get();
        operand->SetBlock(LAST_BLK(func));
        LAST_BLK(func)->SetLabel(std::move(operand));
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
        instr_ptr->SetJmpDest(labels[lbl]);
    } else {
        auto operand = std::make_unique<LabelOperand>(lbl, func->ctx.get_next_id());
        labels[lbl] = operand.get();
        instr_ptr->SetJmpDest(operand.get());
        lbl_store.insert({lbl, std::move(operand)});
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
    for (size_t i : std::views::iota(0u, jlbl.Size())) {
        auto lbl = jlbl.Get(i)->Get<std::string>().value();
        if (!labels.contains(lbl)) {
            // the block for this label has yet not been processed
            auto operand = std::make_unique<LabelOperand>(lbl, func->ctx.get_next_id());
            labels[lbl] = operand.get();
            lbl_store.insert({lbl, std::move(operand)});
        }

        if (i == 0) {
            instr_ptr->SetTrueDest(labels[lbl]);
        } else {
            instr_ptr->SetFalseDest(labels[lbl]);
        }
    }

    // Check conditional value
    auto arg = args.Get(0)->Get<std::string>().value();
    // It is possible that the block that defines the args
    // comes later in text but happens before in control flow
    assert(operands.contains(arg) &&
           "todo: add support of parsing args before def\n");
    instr_ptr->SetOperand(operands[arg].get());

    APPEND_INSTR(func, instr_ptr);

    return func;
}

FuncPtr BrilParser::MakeCallInstruction(FuncPtr func, sjp::Json &instr) {
    auto jfuncs = instr.Get("funcs").value();
    assert(jfuncs.Size() == 1 &&
           "More than one function specified in CallInstruction\n");

    bool has_dest = instr.Get("type").has_value();
    if (has_dest) {
        func = MakeInstruction<CallInstruction, true>(std::move(func), instr);
    } else {
        func = MakeInstruction<CallInstruction, false>(std::move(func), instr);
    }

    auto block = LAST_BLK(func);
    auto iptr = static_cast<CallInstruction *>(LAST_INSTR(block));
    iptr->SetFuncName(jfuncs.Get(0)->Get<std::string>().value());
    iptr->SetRetVal(has_dest);

    if (!has_dest) {
        iptr->AddDest(VoidOperand::GetVoidOperand());
    }

    return func;
}

FuncPtr BrilParser::MakeRetInstruction(FuncPtr func, sjp::Json &instr) {

    func = MakeInstruction<RetInstruction, false>(std::move(func), instr);

    auto block = LAST_BLK(func);
    auto iptr = static_cast<RetInstruction *>(LAST_INSTR(block));

    if (!iptr->GetOperandSize()) {
        iptr->SetOperand(VoidOperand::GetVoidOperand().get());
    }

    return func;
}

// Miscellaneous Instructions
FuncPtr BrilParser::MakeConstInstruction(FuncPtr func, sjp::Json &instr) {
    auto type_str = instr.Get("type")->Get<std::string>().value();
    DataType type = GetDataTypeFromStr(type_str);
    switch (type) {
    case DataType::INT:
        return BuildConstInstruction<IntOperand, DataType::INT>(std::move(func),
                                                                instr);
    case DataType::FLOAT:
        return BuildConstInstruction<FloatOperand, DataType::FLOAT>(
            std::move(func), instr);
    case DataType::BOOL:
        return BuildConstInstruction<BoolOperand, DataType::BOOL>(
            std::move(func), instr);
    default:
        assert(false && "Unexpected type found in MakeConstInstruction\n");
        break;
    }
    return func;
}

FuncPtr BrilParser::ParseInstructions(FuncPtr func, sjp::Json &instr) {
    Opcode opcode = Opcode::NOP;

    // Instruction can be either `op` or `label`
    if (instr.Get("op") != std::nullopt) {
        auto op_str = instr.Get("op")->Get<std::string>().value();
        if (check_label) {
            throw std::runtime_error(
                std::format("Error parsing function {}, expected label but "
                            "found opcode: {}\n",
                            func->GetName(), op_str));
        }
        // op instruction
        opcode = GetOpcodeFromStr(op_str);
    } else if (instr.Get("label") != std::nullopt) {
        // label instruction
        opcode = Opcode::LABEL;
    } else {
        throw std::runtime_error(std::format("Error parsing function {}. "
                                             "No valid instructions found.\n",
                                             func->GetName()));
    }
    check_label = false;

    switch (opcode) {
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
        check_label = true;
        return MakeJmpInstruction(std::move(func), instr);
    }
    case Opcode::BR: {
        // End block
        check_label = true;
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
        return MakeInstruction<FreeInstruction, false>(std::move(func), instr);
    case Opcode::LOAD:
        return MakeInstruction<LoadInstruction>(std::move(func), instr);
    case Opcode::STORE:
        return MakeInstruction<StoreInstruction, false>(std::move(func), instr);
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
        return MakeInstruction<PrintInstruction, false>(std::move(func), instr);
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

// Parse Functions
std::unique_ptr<Function> BrilParser::ParseBody(std::unique_ptr<Function> func,
                                                sjp::Json &instrs) {
    assert(instrs.Size() > 0);

    check_label = false;

    // If instr start with a label then don't create a
    // entry block now. Check later if an entry block is
    // required. Entry block will be required if it is
    // possible to jump to current label
    if (instrs.Get(0)->Get("label") == std::nullopt) {
        // Block starts without a label. Create a new
        // entry label and block. This will also be an
        // unique entry point to the function.
        func = MakeNewBlock(std::move(func), "__sc_entry__");
        func->SetEntryBlock(LAST_BLK(func));
    }

    for (size_t i : std::views::iota(0u, instrs.Size())) {
        auto instr = instrs.Get(i).value();
        func = ParseInstructions(std::move(func), instr);
    }

    return func;
}

FuncPtr BrilParser::ParseArguments(FuncPtr func, sjp::Json &args) {
    // Set the args to true
    func->SetArgs(true);
    func->SetArgsSize(args.Size());

    for (size_t i : std::views::iota(0ul, args.Size())) {
        auto jop = args.Get(i).value();
        std::string name = jop.Get("name")->Get<std::string>().value();
        if (operands.contains(name)) {
            throw std::runtime_error(
                std::format("Error parsing arguments of function {}. "
                            "Redeclaration of argument name {}\n",
                            func->GetName(), name));
        }

        std::shared_ptr<OperandBase> operand = nullptr;
        if (jop.Get("type")->type == sjp::JsonType::jobject) {
            operand = ParsePtrType(std::make_shared<PtrOperand>(name, func->ctx.get_next_id()), jop);
        } else {
            std::string type = jop.Get("type")->Get<std::string>().value();
            operand =
                std::make_shared<RegOperand>(GetDataTypeFromStr(type), name, func->ctx.get_next_id());
        }

        args_vec.push_back(std::make_unique<GetArgInstruction>());
        operands[name] = operand;
        args_vec.back()->AddDest(std::move(operand));
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
    // Parse the args before parsing the body.
    auto args = jfunc.Get("args");
    if (jfunc.Get("args").has_value()) {
        func = ParseArguments(std::move(func), args.value());
    }

    // Parse function body
    auto instrs = jfunc.Get("instrs");
    func = ParseBody(std::move(func), instrs.value());

    // set the args in the first basic block, altough
    // first basic block may not be the unique entry
    // block for this function.
    if (!args_vec.empty()) {
        func->GetBlock(0)->InsertInstructions(std::move(args_vec), 0);
    }

    func->SetEntryBlock(func->GetBlock(0));

    return func;
}

std::unique_ptr<Program> BrilParser::ParseProgram(std::istream &input) {
    auto json_parser = sjp::Parser(input);
    sjp::Json data = json_parser.Parse();

    auto program = std::make_unique<Program>();
    auto functions = data.Get("functions");
    for (size_t i : std::views::iota(0u, functions->Size())) {
        auto jfunc = functions->Get(i).value();
        auto parser = BrilParser();
        program->AddFunction(parser.ParseFunction(jfunc));
    }
    return program;
}
} // namespace sc
