#pragma once

#include "instruction.hpp"
namespace sc {

#define DEFAUL_IMPL                                                            \
    {                                                                          \
        (void)instr;                                                           \
        assert(false);                                                         \
    }

// clang-format off
class InstructionVisitor {
  public:
    // Arithmetic
    virtual void VisitAddInstruction(AddInstruction *instr) DEFAUL_IMPL
    virtual void VisitMulInstruction(MulInstruction *instr) DEFAUL_IMPL
    virtual void VisitSubInstruction(SubInstruction *instr) DEFAUL_IMPL
    virtual void VisitDivInstruction(DivInstruction *instr) DEFAUL_IMPL

    // Comparison
    virtual void VisitEqInstruction(EqInstruction *instr) DEFAUL_IMPL
    virtual void VisitLtInstruction(LtInstruction *instr) DEFAUL_IMPL
    virtual void VisitGtInstruction(GtInstruction *instr) DEFAUL_IMPL
    virtual void VisitLeInstruction(LeInstruction *instr) DEFAUL_IMPL
    virtual void VisitGeInstruction(GeInstruction *instr) DEFAUL_IMPL

    // Logic
    virtual void VisitAndInstruction(AndInstruction *instr) DEFAUL_IMPL
    virtual void VisitOrInstruction(OrInstruction *instr) DEFAUL_IMPL
    virtual void VisitNotInstruction(NotInstruction *instr) DEFAUL_IMPL

    // Control
    virtual void VisitJmpInstruction(JmpInstruction *instr) DEFAUL_IMPL
    virtual void VisitBranchInstruction(BranchInstruction *instr) DEFAUL_IMPL
    virtual void VisitCallInstruction(CallInstruction *instr) DEFAUL_IMPL
    virtual void VisitRetInstruction(RetInstruction *instr) DEFAUL_IMPL

    // SSA
    virtual void VisitSetInstruction(SetInstruction *instr) DEFAUL_IMPL
    virtual void VisitGetInstruction(GetInstruction *instr) DEFAUL_IMPL
    virtual void VisitUndefInstruction(UndefInstruction *instr) DEFAUL_IMPL

    // Memory
    virtual void VisitAllocInstruction(AllocInstruction *instr) DEFAUL_IMPL
    virtual void VisitFreeInstruction(FreeInstruction *instr) DEFAUL_IMPL
    virtual void VisitLoadInstruction(LoadInstruction *instr) DEFAUL_IMPL
    virtual void VisitStoreInstruction(StoreInstruction *instr) DEFAUL_IMPL
    virtual void VisitPtraddInstruction(PtraddInstruction *instr) DEFAUL_IMPL

    // Floating Arithmetic
    virtual void VisitFAddInstruction(FAddInstruction *instr) DEFAUL_IMPL
    virtual void VisitFMulInstruction(FMulInstruction *instr) DEFAUL_IMPL
    virtual void VisitFSubInstruction(FSubInstruction *instr) DEFAUL_IMPL
    virtual void VisitFDivInstruction(FDivInstruction *instr) DEFAUL_IMPL

    // Floating Comparison
    virtual void VisitFEqInstruction(FEqInstruction *instr) DEFAUL_IMPL
    virtual void VisitFLtInstruction(FLtInstruction *instr) DEFAUL_IMPL
    virtual void VisitFGtInstruction(FGtInstruction *instr) DEFAUL_IMPL
    virtual void VisitFLeInstruction(FLeInstruction *instr) DEFAUL_IMPL
    virtual void VisitFGeInstruction(FGeInstruction *instr) DEFAUL_IMPL

    // Miscellanesous
    virtual void VisitIdInstruction(IdInstruction *instr) DEFAUL_IMPL
    virtual void VisitConstInstruction(ConstInstruction *instr) DEFAUL_IMPL
    virtual void VisitPrintInstruction(PrintInstruction *instr) DEFAUL_IMPL
    virtual void VisitNopInstruction(NopInstruction *instr) DEFAUL_IMPL
};
// clang-format on

} // namespace sc
