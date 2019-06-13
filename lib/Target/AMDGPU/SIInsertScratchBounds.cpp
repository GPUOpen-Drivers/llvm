//===- SIInsertScratchBounds.cpp - insert scratch bounds checks           -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Modifications Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
// Notified per clause 4(b) of the license.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This pass inserts bounds checks on scratch accesses.
/// Out-of-bounds reads return zero, and out-of-bounds writes have no effect.
/// This is intended to be used on GCN5 where bounds checking is no longer
/// performed by hardware and hence page faults can results from out-of-bounds
/// accesses by shaders.
///
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPUSubtarget.h"
#include "MCTargetDesc/AMDGPUMCTargetDesc.h"
#include "SIInstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

#include <set>

using namespace llvm;

#define DEBUG_TYPE "si-insert-scratch-bounds"

namespace {

class SIInsertScratchBounds : public MachineFunctionPass {
private:
  const GCNSubtarget *ST;
  const SIInstrInfo *TII;
  MachineRegisterInfo *MRI;
  const SIRegisterInfo *RI;
  std::vector<MachineInstr*> Worklist;

public:
  static char ID;

  SIInsertScratchBounds() : MachineFunctionPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool insertBoundsCheck(MachineFunction &MF, MachineInstr *MI,
                         const int64_t ScratchSize,
                         const unsigned SizeReg,
                         bool &SizeUsed);

  bool runOnMachineFunction(MachineFunction &MF) override;
};

static void zeroReg(MachineBasicBlock &MBB, MachineRegisterInfo *MRI,
                    const SIRegisterInfo *RI, const SIInstrInfo *TII,
                    MachineBasicBlock::iterator &I, const DebugLoc &DL,
                    unsigned Reg) {

  auto EndDstRC = MRI->getRegClass(Reg);
  uint32_t RegSize = RI->getRegSizeInBits(*EndDstRC) / 32;

  assert(RI->isVGPR(*MRI, Reg) && "can only zero VGPRs");

  if (RegSize == 1)
    BuildMI(MBB, I, DL, TII->get(AMDGPU::V_MOV_B32_e32), Reg).addImm(0);
  else {
    SmallVector<unsigned, 8> TRegs;
    for (unsigned i = 0; i < RegSize; ++i) {
      unsigned TReg = MRI->createVirtualRegister(&AMDGPU::VGPR_32RegClass);
      BuildMI(MBB, I, DL, TII->get(AMDGPU::V_MOV_B32_e32), TReg).addImm(0);
      TRegs.push_back(TReg);
    }
    MachineInstrBuilder MIB =
        BuildMI(MBB, I, DL, TII->get(AMDGPU::REG_SEQUENCE), Reg);
    for (unsigned i = 0; i < RegSize; ++i) {
      MIB.addReg(TRegs[i]);
      MIB.addImm(RI->getSubRegFromChannel(i));
    }
  }
}

static void cndmask0Reg(MachineBasicBlock &MBB, MachineRegisterInfo *MRI,
                    const SIRegisterInfo *RI, const SIInstrInfo *TII,
                    MachineBasicBlock::iterator &I, const DebugLoc &DL,
                    unsigned SrcReg, unsigned MaskReg, bool KillMask,
                    unsigned DstReg) {

  auto EndDstRC = MRI->getRegClass(DstReg);
  uint32_t RegSize = RI->getRegSizeInBits(*EndDstRC) / 32;

  assert(RI->isVGPR(*MRI, DstReg) && "can only cndmask VGPRs");

  if (RegSize == 1)
    BuildMI(MBB, I, DL, TII->get(AMDGPU::V_CNDMASK_B32_e64), DstReg)
      .addImm(0)
      .addImm(0)
      .addImm(0)
      .addReg(SrcReg)
      .addReg(MaskReg, getKillRegState(KillMask));
  else {
    SmallVector<unsigned, 8> TRegs;
    for (unsigned i = 0; i < RegSize; ++i) {
      unsigned TReg = MRI->createVirtualRegister(&AMDGPU::VGPR_32RegClass);
      BuildMI(MBB, I, DL, TII->get(AMDGPU::V_CNDMASK_B32_e64), TReg)
        .addImm(0)
        .addImm(0)
        .addImm(0)
        .addReg(SrcReg, 0, AMDGPU::sub0 + i)
        .addReg(MaskReg, getKillRegState(KillMask && (i == (RegSize - 1))));
      TRegs.push_back(TReg);
    }
    MachineInstrBuilder MIB =
        BuildMI(MBB, I, DL, TII->get(AMDGPU::REG_SEQUENCE), DstReg);
    for (unsigned i = 0; i < RegSize; ++i) {
      MIB.addReg(TRegs[i]);
      MIB.addImm(RI->getSubRegFromChannel(i));
    }
  }
}

} // end anonymous namespace

INITIALIZE_PASS(SIInsertScratchBounds, DEBUG_TYPE,
                "SI Insert Scratch Bounds Checks",
                false, false)

char SIInsertScratchBounds::ID = 0;

char &llvm::SIInsertScratchBoundsID = SIInsertScratchBounds::ID;

FunctionPass *llvm::createSIInsertScratchBoundsPass() {
  return new SIInsertScratchBounds;
}

bool SIInsertScratchBounds::insertBoundsCheck(MachineFunction &MF,
                                              MachineInstr *MI,
                                              const int64_t ScratchSize,
                                              const unsigned SizeReg,
                                              bool &SizeUsed) {
  const bool IsLoad = MI->mayLoad();
  DebugLoc DL = MI->getDebugLoc();

  const MachineOperand *Offset =
    TII->getNamedOperand(*MI, AMDGPU::OpName::offset);
  const MachineOperand *VAddr =
    TII->getNamedOperand(*MI, AMDGPU::OpName::vaddr);
  const MachineOperand *Addr =
    VAddr ? VAddr : TII->getNamedOperand(*MI, AMDGPU::OpName::saddr);

  if (!Addr || !Addr->isReg()) {
    // Constant offset -> determine bounds check statically
    if (Offset->getImm() >= ScratchSize) {
      // Statically out-of-bounds -> delete instruction
      if (IsLoad) {
        MachineBasicBlock *MBB = MI->getParent();
        MachineBasicBlock::iterator I(MI);
        MachineOperand &Dst = MI->getOperand(0);
        zeroReg(*MBB, MRI, RI, TII, I, DL, Dst.getReg());
      }
      MI->removeFromParent();
      return true;
    } else {
      // Statically in bounds
      return false;
    }
  }

  // Setup new block structure
  MachineBasicBlock *PreAccessBB = MI->getParent();
  MachineBasicBlock *ScratchAccessBB = MF.CreateMachineBasicBlock();
  MachineBasicBlock *PostAccessBB = MF.CreateMachineBasicBlock();

  MachineFunction::iterator MBBI(*PreAccessBB);
  ++MBBI;

  MF.insert(MBBI, ScratchAccessBB);
  MF.insert(MBBI, PostAccessBB);

  ScratchAccessBB->addSuccessor(PostAccessBB);

  // Move instructions following scratch access to new basic block
  MachineBasicBlock::iterator SuccI(*MI);
  ++SuccI;
  PostAccessBB->transferSuccessorsAndUpdatePHIs(PreAccessBB);
  PostAccessBB->splice(
    PostAccessBB->begin(), PreAccessBB, SuccI, PreAccessBB->end()
  );

  PreAccessBB->addSuccessor(ScratchAccessBB);

  // Move scratch access to its own basic block
  MI->removeFromParent();
  ScratchAccessBB->insertAfter(ScratchAccessBB->begin(), MI);

  MachineBasicBlock::iterator PreI = PreAccessBB->end();
  MachineBasicBlock::iterator PostI = PostAccessBB->begin();
  MachineBasicBlock::iterator ScratchI = ScratchAccessBB->end();
  unsigned AddrReg;
  bool KillAddr = false;

  assert(Addr && Addr->isReg());

  if (Offset && (Offset->getImm() > 0)) {
    AddrReg = MRI->createVirtualRegister(&AMDGPU::VGPR_32RegClass);
    KillAddr = true;

    if (ST->hasAddNoCarry()) {
      BuildMI(*PreAccessBB, PreI, DL, TII->get(AMDGPU::V_ADD_U32_e32), AddrReg)
        .addImm(Offset->getImm())
        .addReg(Addr->getReg());
    } else {
      const unsigned OffsetReg =
        MRI->createVirtualRegister(&AMDGPU::SReg_32RegClass);
      const unsigned UnusedCarry =
        MRI->createVirtualRegister(&AMDGPU::SReg_64RegClass);

      MRI->setRegAllocationHint(UnusedCarry, 0, AMDGPU::VCC);

      BuildMI(*PreAccessBB, PreI, DL, TII->get(AMDGPU::S_MOV_B32), OffsetReg)
        .addImm(Offset->getImm());
      BuildMI(*PreAccessBB, PreI, DL, TII->get(AMDGPU::V_ADD_I32_e64), AddrReg)
        .addReg(UnusedCarry, RegState::Define | RegState::Dead)
        .addReg(Addr->getReg())
        .addReg(OffsetReg, RegState::Kill);
    }
  } else {
    AddrReg = Addr->getReg();
  }

  if (RI->isVGPR(*MRI, AddrReg)) {
    const unsigned CondReg
      = MRI->createVirtualRegister(&AMDGPU::SReg_64_XEXECRegClass);
    const unsigned ExecReg
      = MRI->createVirtualRegister(&AMDGPU::SReg_64_XEXECRegClass);

    BuildMI(*PreAccessBB, PreI, DL,
            TII->get(AMDGPU::V_CMP_LT_U32_e64), CondReg)
      .addReg(AddrReg, getKillRegState(KillAddr))
      .addReg(SizeReg);
    BuildMI(*PreAccessBB, PreI, DL,
            TII->get(AMDGPU::S_AND_SAVEEXEC_B64), ExecReg)
      .addReg(CondReg, getKillRegState(!IsLoad));
    BuildMI(*ScratchAccessBB, ScratchI, DL,
            TII->get(AMDGPU::S_MOV_B64), AMDGPU::EXEC)
      .addReg(ExecReg, RegState::Kill);

    if (IsLoad) {
      MachineOperand &Dst = MI->getOperand(0);
      const unsigned DstReg = Dst.getReg();
      const TargetRegisterClass *DstRC = MRI->getRegClass(DstReg);
      const unsigned LoadDstReg = MRI->createVirtualRegister(DstRC);

      Dst.setReg(LoadDstReg);

      cndmask0Reg(*PostAccessBB, MRI, RI, TII, PostI, DL,
                  LoadDstReg, CondReg, true, DstReg);
    }
  } else {
    if (MI->mayLoad()) {
      // Load -> scalar comparison, then load, else load zero
      MachineBasicBlock *OutOfBoundsBB = MF.CreateMachineBasicBlock();
      MachineBasicBlock::iterator OOBI = OutOfBoundsBB->end();

      MBBI--;
      MF.insert(MBBI, OutOfBoundsBB);
      OutOfBoundsBB->addSuccessor(PostAccessBB);
      PreAccessBB->addSuccessor(OutOfBoundsBB);

      // TODO: mark SCC as clobbered?
      BuildMI(*PreAccessBB, PreI, DL, TII->get(AMDGPU::S_CMP_LT_U32))
        .addReg(AddrReg, getKillRegState(KillAddr))
        .addReg(SizeReg);
      BuildMI(*PreAccessBB, PreI, DL, TII->get(AMDGPU::S_CBRANCH_SCC0))
        .addMBB(OutOfBoundsBB);

      BuildMI(*ScratchAccessBB, ScratchI, DL, TII->get(AMDGPU::S_BRANCH))
        .addMBB(PostAccessBB);

      MachineOperand &Dst = MI->getOperand(0);
      const unsigned DstReg = Dst.getReg();

      const TargetRegisterClass *DstRC = MRI->getRegClass(DstReg);
      const unsigned LoadDstReg = MRI->createVirtualRegister(DstRC);
      const unsigned ZeroDstReg = MRI->createVirtualRegister(DstRC);

      zeroReg(*OutOfBoundsBB, MRI, RI, TII, OOBI, DL, ZeroDstReg);

      BuildMI(*PostAccessBB, PostI, DL, TII->get(TargetOpcode::PHI), DstReg)
        .addReg(LoadDstReg)
        .addMBB(ScratchAccessBB)
        .addReg(ZeroDstReg)
        .addMBB(OutOfBoundsBB);

      Dst.setReg(LoadDstReg);
    } else {
      // Store -> scalar comparison and skip store
      // TODO: mark SCC as clobbered?
      BuildMI(*PreAccessBB, PreI, DL, TII->get(AMDGPU::S_CMP_LT_U32))
        .addReg(AddrReg, getKillRegState(KillAddr))
        .addReg(SizeReg);
      BuildMI(*PreAccessBB, PreI, DL, TII->get(AMDGPU::S_CBRANCH_SCC0))
        .addMBB(PostAccessBB);
      PreAccessBB->addSuccessor(PostAccessBB);
    }
  }

  SizeUsed = true;
  return true;
}

bool SIInsertScratchBounds::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;

  ST = &MF.getSubtarget<GCNSubtarget>();
  TII = ST->getInstrInfo();
  MRI = &MF.getRegInfo();
  RI = ST->getRegisterInfo();

  Worklist.clear();

  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.mayLoad() || MI.mayStore()) {
        for (const auto &MMO : MI.memoperands()) {
          const unsigned AddrSpace = MMO->getPointerInfo().getAddrSpace();
          if (AddrSpace == AMDGPUAS::PRIVATE_ADDRESS) {
            // uses scratch; needs to be processed
            Worklist.push_back(&MI);
            break;
          }
        }
      }
    }
  }

  if (!Worklist.empty()) {
    const MachineFrameInfo &FrameInfo = MF.getFrameInfo();
    const int64_t ScratchSizeEstimate =
      (int64_t) FrameInfo.estimateStackSize(MF);

    const unsigned SizeReg =
      MRI->createVirtualRegister(&AMDGPU::SReg_32RegClass);
    bool SizeUsed = false;

    for (MachineInstr *MI : Worklist) {
      Changed |= insertBoundsCheck(
        MF, MI, ScratchSizeEstimate, SizeReg, SizeUsed
      );
    }

    // If scratch size is required then add to prelude
    if (SizeUsed) {
      MachineBasicBlock *PreludeBB = &MF.front();
      MachineBasicBlock::iterator PreludeI = PreludeBB->begin();
      DebugLoc UnknownDL;

      BuildMI(*PreludeBB, PreludeI, UnknownDL,
              TII->get(AMDGPU::S_MOV_B32), SizeReg)
        .addImm(ScratchSizeEstimate);

      Changed = true;
    }

    Worklist.clear();
  }

  return Changed;
}
