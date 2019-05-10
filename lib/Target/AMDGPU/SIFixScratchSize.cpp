//===- SIFixScratchSize.cpp - resolve scratch size symbols                -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// Modifications Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
// Notified per clause 4(b) of the license.
//
//===----------------------------------------------------------------------===//
//
/// \file
///
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPUSubtarget.h"
#include "MCTargetDesc/AMDGPUMCTargetDesc.h"
#include "SIInstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

#include <set>

using namespace llvm;

#define DEBUG_TYPE "si-fix-scratch-size"

namespace {

class SIFixScratchSize : public MachineFunctionPass {
public:
  static char ID;

  SIFixScratchSize() : MachineFunctionPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
};

} // end anonymous namespace

INITIALIZE_PASS(SIFixScratchSize, DEBUG_TYPE,
                "SI Resolve Scratch Size Symbols",
                false, false)

char SIFixScratchSize::ID = 0;

char &llvm::SIFixScratchSizeID = SIFixScratchSize::ID;

const char *const llvm::SIScratchSizeSymbol = "___SCRATCH_SIZE";

FunctionPass *llvm::createSIFixScratchSizePass() {
  return new SIFixScratchSize;
}

bool SIFixScratchSize::runOnMachineFunction(MachineFunction &MF) {
  const MachineFrameInfo &FrameInfo = MF.getFrameInfo();
  const uint64_t StackSize = FrameInfo.getStackSize();

  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == AMDGPU::S_MOV_B32) {
        MachineOperand& Src = MI.getOperand(1);
        if (Src.isSymbol()) {
          if (strcmp(Src.getSymbolName(), SIScratchSizeSymbol) == 0) {
            LLVM_DEBUG(dbgs() << "Fixing: " << MI << "\n");
            Src.ChangeToImmediate(StackSize);
            Changed = true;
          }
        }
      }
    }
  }

  return Changed;
}
