//===- SIInsertWaterfall.cpp - insert waterall loops at intrinsic markers -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// Replace 3 intrinsics used to mark waterfall regions with actual waterfall
/// loops. This is done at MachineIR level rather than LLVM-IR due to the use of
/// exec mask in this operation.
///
//===----------------------------------------------------------------------===//


#include "AMDGPU.h"
#include "AMDGPUSubtarget.h"
#include "SIInstrInfo.h"
#include "MCTargetDesc/AMDGPUMCTargetDesc.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

#include <set>

using namespace llvm;

#define DEBUG_TYPE "si-insert-waterfall"

namespace {

  static unsigned isWFReadfirstlane(const unsigned Opcode) {
    switch(Opcode) {
    case AMDGPU::SI_WATERFALL_READFIRSTLANE_V1:
      return 1;
    case AMDGPU::SI_WATERFALL_READFIRSTLANE_V2:
      return 2;
    case AMDGPU::SI_WATERFALL_READFIRSTLANE_V4:
      return 4;
    case AMDGPU::SI_WATERFALL_READFIRSTLANE_V8:
      return 8;
    default:
      break;
    }

    return 0; // Not SI_WATERFALL_READFIRSTLANE_*
  }

  static unsigned isWFEnd(const unsigned Opcode) {
    switch(Opcode) {
    case AMDGPU::SI_WATERFALL_END_V1:
      return 1;
    case AMDGPU::SI_WATERFALL_END_V2:
      return 2;
    case AMDGPU::SI_WATERFALL_END_V4:
      return 4;
    case AMDGPU::SI_WATERFALL_END_V8:
      return 8;
    default:
      break;
    }

    return 0; // Not SI_WATERFALL_READFIRSTLANE_*
  }

  static void initReg(MachineBasicBlock &MBB, MachineRegisterInfo *MRI,
                      const SIRegisterInfo *RI, const SIInstrInfo *TII,
                      MachineBasicBlock::iterator &I,
                      const DebugLoc &DL, unsigned Reg, unsigned ImmVal) {

    auto EndDstRC = MRI->getRegClass(Reg);
    uint32_t RegSize = RI->getRegSizeInBits(*EndDstRC) / 32;

    if (RegSize == 1)
      BuildMI(MBB, I, DL, TII->get(AMDGPU::V_MOV_B32_e32), Reg)
        .addImm(0);
    else {
      SmallVector<unsigned, 8> TRegs;
      for ( unsigned i = 0; i < RegSize ; i++) {
        unsigned TReg = MRI->createVirtualRegister(&AMDGPU::VGPR_32RegClass);
        BuildMI(MBB, I, DL, TII->get(AMDGPU::V_MOV_B32_e32), TReg)
          .addImm(ImmVal);
        TRegs.push_back(TReg);
      }
      MachineInstrBuilder MIB =
        BuildMI(MBB, I, DL, TII->get(AMDGPU::REG_SEQUENCE), Reg);
      for (unsigned i = 0; i < RegSize ; ++i) {
        MIB.addReg(TRegs[i]);
        MIB.addImm(RI->getSubRegFromChannel(i));
      }
    }
  }

  static void orReg(MachineBasicBlock &MBB, MachineRegisterInfo *MRI,
                    const SIRegisterInfo *RI, const SIInstrInfo *TII,
                    MachineBasicBlock::iterator &I,
                    const DebugLoc &DL,
                    unsigned EndDst, unsigned PhiReg, unsigned EndSrc) {
    auto EndDstRC = MRI->getRegClass(EndDst);
    uint32_t RegSize = RI->getRegSizeInBits(*EndDstRC) / 32;

    if (RegSize == 1)
      BuildMI(MBB, I, DL, TII->get(AMDGPU::V_OR_B32_e64), EndDst)
        .addReg(PhiReg)
        .addReg(EndSrc);
    else {
      SmallVector<unsigned, 8> TRegs;
      for (unsigned i = 0 ; i < RegSize ; ++i) {
        unsigned TReg = MRI->createVirtualRegister(&AMDGPU::VGPR_32RegClass);
        BuildMI(MBB, I, DL, TII->get(AMDGPU::V_OR_B32_e64), TReg)
          .addReg(PhiReg, 0, AMDGPU::sub0 + i)
          .addReg(EndSrc, 0, AMDGPU::sub0 + i);
        TRegs.push_back(TReg);
      }
      MachineInstrBuilder MIB =
        BuildMI(MBB, I, DL, TII->get(AMDGPU::REG_SEQUENCE), EndDst);
      for ( unsigned i = 0; i < RegSize ; i++) {
        MIB.addReg(TRegs[i]);
        MIB.addImm(RI->getSubRegFromChannel(i));
      }
    }
  }

  static void readFirstLaneReg(MachineBasicBlock &MBB, MachineRegisterInfo *MRI,
                               const SIRegisterInfo *RI, const SIInstrInfo *TII,
                               MachineBasicBlock::iterator &I,
                               const DebugLoc &DL,
                               unsigned RFLReg, unsigned RFLSrcReg,
                               MachineOperand &RFLSrcOp) {
    auto RFLRegRC = MRI->getRegClass(RFLReg);
    uint32_t RegSize = RI->getRegSizeInBits(*RFLRegRC) / 32;

    if (RegSize == 1)
      BuildMI(MBB, I, DL, TII->get(AMDGPU::V_READFIRSTLANE_B32), RFLReg)
        .addReg(RFLSrcReg, getUndefRegState(RFLSrcOp.isUndef()));
    else {
      SmallVector<unsigned, 8> TRegs;
      for (unsigned i = 0 ; i < RegSize ; ++i) {
        unsigned TReg = MRI->createVirtualRegister(&AMDGPU::SGPR_32RegClass);
        BuildMI(MBB, I, DL, TII->get(AMDGPU::V_READFIRSTLANE_B32), TReg)
          .addReg(RFLSrcReg, 0, AMDGPU::sub0 + i);
        TRegs.push_back(TReg);
      }
      MachineInstrBuilder MIB =
        BuildMI(MBB, I, DL, TII->get(AMDGPU::REG_SEQUENCE), RFLReg);
      for ( unsigned i = 0 ; i < RegSize ; ++i) {
        MIB.addReg(TRegs[i]);
        MIB.addImm(RI->getSubRegFromChannel(i));
      }
    }
  }

  class SIInsertWaterfall : public MachineFunctionPass {
  private:

    struct WaterfallWorkitem {
      MachineInstr *Begin;
      const SIInstrInfo *TII;
      unsigned TokReg;
      MachineInstr *Final;

      std::vector<MachineInstr*> RFLList;
      std::vector<MachineInstr*> EndList;
      // List of corresponding init, newdst and phi registers used in loop for
      // end pseudos
      std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>> EndRegs;
      std::vector<unsigned> RFLRegs;

      WaterfallWorkitem() = default;
      WaterfallWorkitem(MachineInstr *_Begin, const SIInstrInfo *_TII) :
        Begin(_Begin), TII(_TII), Final(nullptr) {

        auto TokMO = TII->getNamedOperand(*Begin, AMDGPU::OpName::tok);
        assert(TokMO &&
               "Unable to extract tok operand from SI_WATERFALL_BEGIN pseudo op");
        TokReg = TokMO->getReg();
      };

      bool addCandidate(MachineInstr *Cand) {
        unsigned Opcode = Cand->getOpcode();

        if (isWFReadfirstlane(Opcode) || isWFEnd(Opcode)) {
          auto CandTokMO = TII->getNamedOperand(*Cand, AMDGPU::OpName::tok);
          unsigned CandTokReg = CandTokMO->getReg();
          if (CandTokReg == TokReg) {
            if (isWFReadfirstlane(Opcode)) {
              RFLList.push_back(Cand);
              return true;
            } else {
              EndList.push_back(Cand);
              Final = Cand;
              return true;
            }
          }
        } else {
          llvm_unreachable("Candidate is not an SI_WATERFALL_* pseudo instruction");
        }
        return false;
      }
    };

    std::vector<WaterfallWorkitem> Worklist;

    const SISubtarget *ST;
    const SIInstrInfo *TII;
    MachineRegisterInfo *MRI;
    const SIRegisterInfo *RI;

  public:
    static char ID;

    SIInsertWaterfall() : MachineFunctionPass(ID) {
      initializeSIInsertWaterfallPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      MachineFunctionPass::getAnalysisUsage(AU);
    }

    bool processWaterfall(MachineBasicBlock &MBB);

    bool runOnMachineFunction(MachineFunction &MF) override;
  };

} // End anonymous namespace.

INITIALIZE_PASS(SIInsertWaterfall, DEBUG_TYPE, "SI Insert waterfalls", false, false)

char SIInsertWaterfall::ID = 0;

char &llvm::SIInsertWaterfallID = SIInsertWaterfall::ID;

FunctionPass *llvm::createSIInsertWaterfallPass() {
  return new SIInsertWaterfall;
}

bool SIInsertWaterfall::processWaterfall(MachineBasicBlock &MBB) {
  bool Changed = false;
  MachineFunction &MF = *MBB.getParent();

  // Firstly we check that there are at least 3 related waterfall instructions
  // for this begin
  // SI_WATERFALL_BEGIN [ SI_WATERFALL_READFIRSTLANE ]+ [ SI_WATERFALL_END ]+
  // If there are multiple waterfall loops they must also be disjoint

  for (WaterfallWorkitem &Item : Worklist ) {
    LLVM_DEBUG(dbgs() << "Processing " << Item.Begin << "\n");

    for ( auto RUse = MRI->use_begin(Item.TokReg), RSE = MRI->use_end();
          RUse != RSE; ++RUse) {
      MachineInstr *RUseMI = RUse->getParent();
      assert((MBB.getNumber() == RUseMI->getParent()->getNumber()) &&
             "Linked WATERFALL pseudo ops found in different BBs");
    }

    assert(Item.RFLList.size() && Item.EndList.size() &&
           "SI_WATERFALL* pseudo instruction group must have at least 1 of each type");

    // Insert the waterfall loop code around the identified region of
    // instructions
    // Loop starts at the SI_WATERFALL_BEGIN
    // SI_WATERFALL_READFIRSTLANE is replaced with appropriate readfirstlane
    // instructions OR is removed
    // if the readfirstlane is using the same index as the SI_WATERFALL_BEGIN
    // Loop is ended after the last SI_WATERFALL_END and these instructions are
    // removed with the src replacing all dst uses
    auto Index = TII->getNamedOperand(*(Item.Begin), AMDGPU::OpName::idx);

    MachineBasicBlock::iterator I(Item.Begin);
    const DebugLoc &DL = Item.Begin->getDebugLoc();

    // Initialize the register we accumulate the result into, which is the
    // target of any SI_WATERFALL_END instruction
    for (auto EndMI : Item.EndList) {
      // TODO:: Clearly this needs to cope with different size values
      auto EndDst = TII->getNamedOperand(*EndMI, AMDGPU::OpName::dst)->getReg();
      auto EndDstRC = MRI->getRegClass(EndDst);
      unsigned EndInit = MRI->createVirtualRegister(EndDstRC);
      unsigned PhiReg = MRI->createVirtualRegister(EndDstRC);
      unsigned EndSrc = TII->getNamedOperand(*EndMI,
                                             AMDGPU::OpName::src)->getReg();
      initReg(MBB, MRI, RI, TII, I, DL, EndInit, 0x0);
      //BuildMI(MBB, I, DL, TII->get(AMDGPU::V_MOV_B32_e32), EndInit)
      //  .addImm(0x0);
      Item.EndRegs.push_back(std::make_tuple(EndInit, EndDst, PhiReg, EndSrc));
    }

    // TODO: Need to cope with different exec sizes such as for Wave32
    unsigned SaveExec =
      MRI->createVirtualRegister(&AMDGPU::SReg_64_XEXECRegClass);
    unsigned TmpExec =
      MRI->createVirtualRegister(&AMDGPU::SReg_64_XEXECRegClass);

    BuildMI(MBB, I, DL, TII->get(TargetOpcode::IMPLICIT_DEF), TmpExec);

    // Save the EXEC mask
    BuildMI(MBB, I, DL, TII->get(AMDGPU::S_MOV_B64), SaveExec)
      .addReg(AMDGPU::EXEC);

    MachineBasicBlock &LoopBB = *MF.CreateMachineBasicBlock();
    MachineBasicBlock &RemainderBB = *MF.CreateMachineBasicBlock();
    MachineFunction::iterator MBBI(MBB);
    ++MBBI;

    MF.insert(MBBI, &LoopBB);
    MF.insert(MBBI, &RemainderBB);

    LoopBB.addSuccessor(&LoopBB);
    LoopBB.addSuccessor(&RemainderBB);

    // Move all instructions from the SI_WATERFALL_BEGIN to the last
    // SI_WATERFALL_END into the new LoopBB
    MachineBasicBlock::iterator SpliceE(Item.Final);
    ++SpliceE;
    LoopBB.splice(LoopBB.begin(), &MBB, I, SpliceE);

    // Iterate over the instructions inserted into the loop
    // Need to unset any kill flag on any uses as now this is a loop that is no
    // longer valid
    for (MachineInstr &MI : LoopBB) {
      MI.clearKillInfo();
    }

    RemainderBB.transferSuccessorsAndUpdatePHIs(&MBB);
    RemainderBB.splice(RemainderBB.begin(), &MBB, SpliceE, MBB.end());
    MachineBasicBlock::iterator E(Item.Final);
    ++E;

    MBB.addSuccessor(&LoopBB);

    MachineBasicBlock::iterator J = LoopBB.begin();

    unsigned PhiExec = MRI->createVirtualRegister(&AMDGPU::SReg_64_XEXECRegClass);
    unsigned NewExec = MRI->createVirtualRegister(&AMDGPU::SReg_64_XEXECRegClass);
    unsigned CurrentIdxReg = MRI->createVirtualRegister(&AMDGPU::SGPR_32RegClass);
    unsigned CondReg = MRI->createVirtualRegister(&AMDGPU::SReg_64_XEXECRegClass);

    for (auto EndReg : Item.EndRegs) {
      BuildMI(LoopBB, J, DL, TII->get(TargetOpcode::PHI), std::get<2>(EndReg))
        .addReg(std::get<0>(EndReg))
        .addMBB(&MBB)
        .addReg(std::get<1>(EndReg))
        .addMBB(&LoopBB);
    }
    BuildMI(LoopBB, J, DL, TII->get(TargetOpcode::PHI), PhiExec)
      .addReg(TmpExec)
      .addMBB(&MBB)
      .addReg(NewExec)
      .addMBB(&LoopBB);

    // Get the next index to use from the first enabled lane
    BuildMI(LoopBB, J, DL, TII->get(AMDGPU::V_READFIRSTLANE_B32), CurrentIdxReg)
      .addReg(Index->getReg(), getUndefRegState(Index->isUndef()));

    // Also process the readlane pseudo ops - if readfirstlane is using the index
    // then just replace with the CurrentIdxReg instead
    for (auto RFLMI : Item.RFLList) {
      auto RFLSrcOp = TII->getNamedOperand(*RFLMI, AMDGPU::OpName::src);
      auto RFLDstOp = TII->getNamedOperand(*RFLMI, AMDGPU::OpName::dst);
      unsigned RFLSrcReg = RFLSrcOp->getReg();
      unsigned RFLDstReg = RFLDstOp->getReg();

      if (RFLSrcReg == Index->getReg()) {
        // Use the CurrentIdxReg for this
        Item.RFLRegs.push_back(CurrentIdxReg);
        MRI->replaceRegWith(RFLDstReg, CurrentIdxReg);
      } else {
        Item.RFLRegs.push_back(RFLDstReg);
        // Insert function to expand to required size here
        readFirstLaneReg(LoopBB, MRI, RI, TII, J, DL, RFLDstReg,
                         RFLSrcReg, *RFLSrcOp);
      }
    }

    // Compare the jusr read idx value to all possible idx values
    BuildMI(LoopBB, J, DL, TII->get(AMDGPU::V_CMP_EQ_U32_e64), CondReg)
      .addReg(CurrentIdxReg)
      .addReg(Index->getReg(), 0, Index->getSubReg());

    // Update EXEC, save the original EXEC value to VCC
    BuildMI(LoopBB, J, DL, TII->get(AMDGPU::S_AND_SAVEEXEC_B64), NewExec)
      .addReg(CondReg, RegState::Kill);

    // TODO: Conditional branch here to loop header as potential optimization?

    // Move the just read value into the destination using OR
    // TODO: In theory a mov would do here - but this is tricky to get to work
    // correctly as it seems to confuse the regsiter allocator and other passes
    for (auto EndReg : Item.EndRegs) {
      MachineBasicBlock::iterator EndInsert(Item.Final);
      orReg(LoopBB, MRI, RI, TII, EndInsert, DL, std::get<1>(EndReg),
            std::get<2>(EndReg), std::get<3>(EndReg));
    }

    MRI->setSimpleHint(NewExec, CondReg);

    // Update EXEC, switch all done bits to 0 and all todo bits to 1.
    BuildMI(LoopBB, E, DL, TII->get(AMDGPU::S_XOR_B64), AMDGPU::EXEC)
      .addReg(AMDGPU::EXEC)
      .addReg(NewExec);

    // XXX - s_xor_b64 sets scc to 1 if the result is nonzero, so can we use
    // s_cbranch_scc0?

    // Loop back if there are still variants to cover
    BuildMI(LoopBB, E, DL, TII->get(AMDGPU::S_CBRANCH_EXECNZ))
      .addMBB(&LoopBB);

    MachineBasicBlock::iterator First = RemainderBB.begin();
    BuildMI(RemainderBB, First, DL, TII->get(AMDGPU::S_MOV_B64), AMDGPU::EXEC)
      .addReg(SaveExec);

    Item.Begin->eraseFromParent();
    for (auto RFLMI : Item.RFLList)
      RFLMI->eraseFromParent();
    for (auto ENDMI : Item.EndList)
      ENDMI->eraseFromParent();

    Changed = true;
  }
  return Changed;
}

bool SIInsertWaterfall::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;

  ST = &MF.getSubtarget<SISubtarget>();
  TII = ST->getInstrInfo();
  MRI = &MF.getRegInfo();
  RI = ST->getRegisterInfo();


  for (MachineBasicBlock &MBB : MF) {
    Worklist.clear();

    for (MachineInstr &MI : MBB) {
      unsigned Opcode = MI.getOpcode();
      if (Opcode == AMDGPU::SI_WATERFALL_BEGIN)
        Worklist.push_back(WaterfallWorkitem(&MI, TII));
      else if (isWFReadfirstlane(Opcode) || isWFEnd(Opcode))
        if (!Worklist.back().addCandidate(&MI)) {
          llvm_unreachable("Overlapping SI_WATERFALL_* groups");
        }
    }
    Changed |= processWaterfall(MBB);
  }

  return Changed;
}
