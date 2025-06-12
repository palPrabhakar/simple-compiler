#pragma once

#include "instruction.hpp"
namespace sc {

#define DEFAULT_IMPL                                                            \
    {                                                                          \
        (void)instr;                                                           \
        assert(false);                                                         \
    }

// clang-format off
class InstructionVisitor {
  public:
    // Arithmetic
    virtual void VisitAddInstruction(AddInstruction *instr) DEFAULT_IMPL
    virtual void VisitMulInstruction(MulInstruction *instr) DEFAULT_IMPL
    virtual void VisitSubInstruction(SubInstruction *instr) DEFAULT_IMPL
    virtual void VisitDivInstruction(DivInstruction *instr) DEFAULT_IMPL

    // Comparison
    virtual void VisitEqInstruction(EqInstruction *instr) DEFAULT_IMPL
    virtual void VisitLtInstruction(LtInstruction *instr) DEFAULT_IMPL
    virtual void VisitGtInstruction(GtInstruction *instr) DEFAULT_IMPL
    virtual void VisitLeInstruction(LeInstruction *instr) DEFAULT_IMPL
    virtual void VisitGeInstruction(GeInstruction *instr) DEFAULT_IMPL

    // Logic
    virtual void VisitAndInstruction(AndInstruction *instr) DEFAULT_IMPL
    virtual void VisitOrInstruction(OrInstruction *instr) DEFAULT_IMPL
    virtual void VisitNotInstruction(NotInstruction *instr) DEFAULT_IMPL

    // Control
    virtual void VisitJmpInstruction(JmpInstruction *instr) DEFAULT_IMPL
    virtual void VisitBranchInstruction(BranchInstruction *instr) DEFAULT_IMPL
    virtual void VisitCallInstruction(CallInstruction *instr) DEFAULT_IMPL
    virtual void VisitRetInstruction(RetInstruction *instr) DEFAULT_IMPL

    // SSA
    virtual void VisitSetInstruction(SetInstruction *instr) DEFAULT_IMPL
    virtual void VisitGetInstruction(GetInstruction *instr) DEFAULT_IMPL
    virtual void VisitUndefInstruction(UndefInstruction *instr) DEFAULT_IMPL

    // Memory
    virtual void VisitAllocInstruction(AllocInstruction *instr) DEFAULT_IMPL
    virtual void VisitFreeInstruction(FreeInstruction *instr) DEFAULT_IMPL
    virtual void VisitLoadInstruction(LoadInstruction *instr) DEFAULT_IMPL
    virtual void VisitStoreInstruction(StoreInstruction *instr) DEFAULT_IMPL
    virtual void VisitPtraddInstruction(PtraddInstruction *instr) DEFAULT_IMPL

    // Floating Arithmetic
    virtual void VisitFAddInstruction(FAddInstruction *instr) DEFAULT_IMPL
    virtual void VisitFMulInstruction(FMulInstruction *instr) DEFAULT_IMPL
    virtual void VisitFSubInstruction(FSubInstruction *instr) DEFAULT_IMPL
    virtual void VisitFDivInstruction(FDivInstruction *instr) DEFAULT_IMPL

    // Floating Comparison
    virtual void VisitFEqInstruction(FEqInstruction *instr) DEFAULT_IMPL
    virtual void VisitFLtInstruction(FLtInstruction *instr) DEFAULT_IMPL
    virtual void VisitFGtInstruction(FGtInstruction *instr) DEFAULT_IMPL
    virtual void VisitFLeInstruction(FLeInstruction *instr) DEFAULT_IMPL
    virtual void VisitFGeInstruction(FGeInstruction *instr) DEFAULT_IMPL

    // Miscellanesous
    virtual void VisitIdInstruction(IdInstruction *instr) DEFAULT_IMPL
    virtual void VisitConstInstruction(ConstInstruction *instr) DEFAULT_IMPL
    virtual void VisitPrintInstruction(PrintInstruction *instr) DEFAULT_IMPL
    virtual void VisitNopInstruction(NopInstruction *instr) DEFAULT_IMPL

    // Internal
    virtual void VisitGetArgInstruction(GetArgInstruction *instr) DEFAULT_IMPL
};
// clang-format on

} // namespace sc
