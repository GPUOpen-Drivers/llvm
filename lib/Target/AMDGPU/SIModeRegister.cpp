//===-- SIModeRegister.cpp - Mode Register --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
/// The pass inserts changes to the Mode register settings as required.
/// Currently only the double precision floating point rounding mode setting is
/// handled.
//===----------------------------------------------------------------------===//
//
#include "AMDGPU.h"
#include "AMDGPUSubtarget.h"
#include "SIInstrInfo.h"
#include "AMDGPUInstrInfo.h"
#include "SIMachineFunctionInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

#define DEBUG_TYPE "si-mode-register"

STATISTIC(NumSetregInserted,
          "Number of setreg of mode register inserted.");

using namespace llvm;

namespace {

class SIModeRegister : public MachineFunctionPass {
public:
  static char ID;
  unsigned stop;

public:
  SIModeRegister() : MachineFunctionPass(ID) {
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
};
} // End anonymous namespace.

INITIALIZE_PASS(SIModeRegister, DEBUG_TYPE,
                "insert updates to mode register settings", false, false)

char SIModeRegister::ID = 0;

char &llvm::SIModeRegisterID = SIModeRegister::ID;

FunctionPass *llvm::createSIModeRegisterPass() {
  return new SIModeRegister();
}

// We iterate through the instructions of each block and for any that use the
// FP DP rounding mode we check that the current mode is appropropriate. If
// not we insert a setreg to change it. If we find a setreg that modifies the
// rounding mode we track that as the current value.
// We then recursively propagate the final value to all the successor blocks.
// For back-edges we need to revisit blocks until we revisit a block and find
// an instruction that uses the DP rounding mode or as setreg that modifies it
// (in those cases we know successor blocks already have the required modes set)
// or we visit a block for the second time (we know there are no instructions
// that use or set the FP DP rounding mode)
static int processBlock(MachineBasicBlock &MBB, const SIInstrInfo *TII,
                        int currentMode, SmallVector<unsigned, 32> &revisits) {
  MachineBasicBlock::iterator I, Next;
  for (I = MBB.SkipPHIsLabelsAndDebug(MBB.begin()); I != MBB.end(); I = Next) {
    Next = std::next(I);
    MachineInstr &MI = *I;
    if (TII->usesFPDPRounding(MI)) {
      // This instruction uses the DP rounding mode - check that the current
      // mode is suitable, and if not insert a setreg to change the mode
      if ((MI.getOpcode() == AMDGPU::V_INTERP_P1LL_F16) ||
          (MI.getOpcode() == AMDGPU::V_INTERP_P1LV_F16) ||
          (MI.getOpcode() == AMDGPU::V_INTERP_P2_F16)) {
        // f16 interpolation instructions need round to zero
        if (currentMode != FP_ROUND_ROUND_TO_ZERO) {
          currentMode = FP_ROUND_ROUND_TO_ZERO;
          BuildMI(MBB, I, 0, TII->get(AMDGPU::S_SETREG_IMM32_B32))
                 .addImm(currentMode).addImm(0x881);
          ++NumSetregInserted;
        }
      } else {
        // By default we use round to nearest for other DP instructions
        // NOTE: this should come from a per function rounding mode setting once
        // such a setting exists.
        if (currentMode != FP_ROUND_ROUND_TO_NEAREST) {
          currentMode = FP_ROUND_ROUND_TO_NEAREST;
          BuildMI(MBB, I, 0, TII->get(AMDGPU::S_SETREG_IMM32_B32))
                 .addImm(currentMode).addImm(0x881);
          ++NumSetregInserted;
        }
      }
      if (revisits[MBB.getNumber()] >= 1)
        return currentMode;
    } else if ((MI.getOpcode() == AMDGPU::S_SETREG_B32) ||
               (MI.getOpcode() == AMDGPU::S_SETREG_IMM32_B32)) {
      // track changes to the rounding mode

      // ignore setreg if not writing to MODE register
      unsigned dst = TII->getNamedOperand(MI, AMDGPU::OpName::simm16)->getImm();
      if (((dst & AMDGPU::Hwreg::ID_MASK_) >> AMDGPU::Hwreg::ID_SHIFT_) !=
           AMDGPU::Hwreg::ID_MODE)
        continue;

      unsigned width = ((dst & AMDGPU::Hwreg::WIDTH_M1_MASK_) >>
                        AMDGPU::Hwreg::WIDTH_M1_SHIFT_) + 1;
      unsigned offset = (dst & AMDGPU::Hwreg::OFFSET_MASK_) >>
                         AMDGPU::Hwreg::OFFSET_SHIFT_;
      unsigned mask = ((1 << width) - 1) << offset;

      // skip if not updating any part of the DP rounding mode
      if ((mask & FP_ROUND_MODE_DP(3)) == 0)
        continue;
      // it is possible for the setreg to update only part of the DP mode
      // field so we'll mask the current and new modes appropriately -
      // however, if we don't know the current mode we can't use a partial
      // value
      bool partial = ((mask & FP_ROUND_MODE_DP(3)) != FP_ROUND_MODE_DP(3));
      if (partial && (currentMode == -1))
        continue;
      if (MI.getOpcode() == AMDGPU::S_SETREG_IMM32_B32) {
        unsigned val = TII->getNamedOperand(MI, AMDGPU::OpName::imm)->getImm();
        currentMode = (((val << offset) & FP_ROUND_MODE_DP(3)) |
                      ((FP_ROUND_MODE_DP(currentMode) & ~mask))) >> 2;
      } else {
        currentMode = -1;
      }
      // if it was a partial update we may have a different currentMode from
      // values via different paths so we need to continue the propagation,
      // otherwise if we are revisiting the block we can return
      if ((revisits[MBB.getNumber()] >= 1 ) && !partial)
        return currentMode;
    }
  }

  // propagate the current mode to all successor blocks
  if (revisits[MBB.getNumber()] < 2) {
    ++revisits[MBB.getNumber()];
    MachineBasicBlock::succ_iterator S;
    for (MachineBasicBlock::succ_iterator S = MBB.succ_begin(), E = MBB.succ_end();
        S != E; S = std::next(S)) {
      MachineBasicBlock &B = *(*S);
      processBlock(B, TII, currentMode, revisits);
    }
    --revisits[MBB.getNumber()];
  }
  return currentMode;
}

// The DP Rounding flags within the Mode register are used to control both
// 64 bit and 16 bit floating point rounding behavior.
// The 16 bit interpolation instructions require Round to Zero for correct
// results, so explicit mode changes may need to be inserted to ensure
// each instruction has the required mode.
// Other mode register settings may need to be tracked in the future.
bool SIModeRegister::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(MF.getFunction()))
    return false;

  SmallVector<unsigned, 32> revisits;
  revisits.resize(MF.getNumBlockIDs());
  const SISubtarget &ST = MF.getSubtarget<SISubtarget>();
  const SIInstrInfo *TII = ST.getInstrInfo();
  MachineFunction::iterator BI = MF.begin();
  // We currently assume the default rounding mode is Round to Nearest
  // NOTE: this should come from a per function rounding mode setting once such
  // a setting exists.
  processBlock(*BI, TII, FP_ROUND_ROUND_TO_NEAREST, revisits);

  return NumSetregInserted > 0;
}
