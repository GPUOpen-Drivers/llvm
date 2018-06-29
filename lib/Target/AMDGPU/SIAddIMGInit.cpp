//===-- SIAddIMGInit.cpp - Add any required IMG inits ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
/// Any MIMG instructions that use tfe or lwe require an initialization of the
/// result register that will be written in the case of a memory access failure
/// The required code is also added to tie this init code to the result of the
/// img instruction
///
//===----------------------------------------------------------------------===//
//

#define DEBUG_TYPE "si-img-init"
#include "AMDGPU.h"
#include "AMDGPUSubtarget.h"
#include "SIInstrInfo.h"
#include "MCTargetDesc/AMDGPUMCTargetDesc.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

namespace {

class SIAddIMGInit : public MachineFunctionPass {
public:
  static char ID;

public:
  SIAddIMGInit() : MachineFunctionPass(ID) {
    initializeSIAddIMGInitPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return "SI Add IMG init"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
};

} // End anonymous namespace.

INITIALIZE_PASS(SIAddIMGInit, DEBUG_TYPE,
                "SI Add IMG Init", false, false)

char SIAddIMGInit::ID = 0;

char &llvm::SIAddIMGInitID = SIAddIMGInit::ID;

FunctionPass *llvm::createSIAddIMGInitPass() {
  return new SIAddIMGInit();
}

bool SIAddIMGInit::runOnMachineFunction(MachineFunction &MF) {
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const GCNSubtarget &ST = MF.getSubtarget<GCNSubtarget>();
  const SIInstrInfo *TII = ST.getInstrInfo();
  const SIRegisterInfo *RI = ST.getRegisterInfo();
  bool Changed = false;

  for (MachineFunction::iterator BI = MF.begin(), BE = MF.end();
                                                  BI != BE; ++BI) {
    MachineBasicBlock &MBB = *BI;
    MachineBasicBlock::iterator I, Next;
    for (I = MBB.begin(); I != MBB.end(); I = Next) {
      Next = std::next(I);
      MachineInstr &MI = *I;

      auto Opcode = MI.getOpcode();
      if (TII->isMIMG(Opcode) && !TII->get(Opcode).mayStore()) {
        MachineOperand *tfe = TII->getNamedOperand(MI, AMDGPU::OpName::tfe);
        MachineOperand *lwe = TII->getNamedOperand(MI, AMDGPU::OpName::lwe);
        MachineOperand *d16 = TII->getNamedOperand(MI, AMDGPU::OpName::d16);

        // Abandon attempts for instructions that don't have tfe or lwe fields
        // Shouldn't be any at this point, but this will allow for future
        // variants.
        if (!tfe && !lwe)
          continue;

        unsigned tfeVal = tfe->getImm();
        unsigned lweVal = lwe->getImm();
        unsigned d16Val = d16 ? d16->getImm() : 0;

        if (tfeVal || lweVal) {
          // At least one of TFE or LWE are non-zero
          // We have to insert a suitable initialization of the result value and
          // tie this to the dest of the image instruction.

          const DebugLoc &DL = MI.getDebugLoc();

          int dstIdx = AMDGPU::getNamedOperandIdx(MI.getOpcode(),
                                                  AMDGPU::OpName::vdata);

          // Calculate which dword we have to initialize to 0.
          MachineOperand *MO_Dmask =
            TII->getNamedOperand(MI, AMDGPU::OpName::dmask);
          // Abandon attempt if no dmask operand is found.
          if (!MO_Dmask) continue;

          unsigned dmask = MO_Dmask->getImm();
          // Determine the number of active lanes taking into account the
          // Gather4 special case
          unsigned activeLanes =
            TII->isGather4(Opcode) ? 4 : countPopulation(dmask);
          // Subreg indices are counted from 1
          // When D16 then we want next whole VGPR after write data.
          bool Packed = !ST.hasUnpackedD16VMem();
          unsigned initIdx =
            d16Val && Packed ? ((activeLanes + 1) >> 1) + 1
                             : activeLanes + 1;

          // Abandon attempt if the dst size isn't large enough
          // - this is in fact an error but this is picked up elsewhere and
          // reported correctly.
          uint32_t dstSize =
            RI->getRegSizeInBits(*TII->getOpRegClass(MI, dstIdx)) / 32;
          if (dstSize < initIdx) continue;

          // Create a register for the intialization value.
          unsigned prevDst =
            MRI.createVirtualRegister(TII->getOpRegClass(MI, dstIdx));
          unsigned newDst = 0; // Final initialized value will be in here

          // If PRTStrictNull feature is enabled (the default) then initialize
          // all the result registers to 0, otherwise just the error indication
          // register (VGPRn+1)
          unsigned sizeLeft = ST.usePRTStrictNull() ? initIdx : 1;
          unsigned currIdx = ST.usePRTStrictNull() ? 1 : initIdx;

          if (dstSize == 1) {
            // In this case we can just initialize the result directly
            BuildMI(MBB, MI, DL, TII->get(AMDGPU::V_MOV_B32_e32), prevDst)
                .addImm(0);
            newDst = prevDst;
          } else {
            BuildMI(MBB, MI, DL, TII->get(AMDGPU::IMPLICIT_DEF), prevDst);
            for (; sizeLeft; sizeLeft--, currIdx++) {
              newDst =
                  MRI.createVirtualRegister(TII->getOpRegClass(MI, dstIdx));
              // Initialize dword
              unsigned subReg =
                  MRI.createVirtualRegister(&AMDGPU::VGPR_32RegClass);
              BuildMI(MBB, MI, DL, TII->get(AMDGPU::V_MOV_B32_e32), subReg)
                  .addImm(0);
              // Insert into the super-reg
              BuildMI(MBB, I, DL, TII->get(TargetOpcode::INSERT_SUBREG), newDst)
                  .addReg(prevDst)
                  .addReg(subReg)
                  .addImm(currIdx);

              prevDst = newDst;
            }
          }

          // Add as an implicit operand
          MachineInstrBuilder(MF,MI).addReg(newDst, RegState::Implicit);

          // Tie the just added implicit operand to the dst
          MI.tieOperands(dstIdx, MI.getNumOperands() - 1);

          Changed = true;
        }
      }
    }
  }

  return Changed;
}
