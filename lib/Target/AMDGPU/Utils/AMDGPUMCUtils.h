//===- AMDGPUMCUtils.h - MachineIR utils for AMDGPU -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// Modifications Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
// Notified per clause 4(b) of the license.
//
//===----------------------------------------------------------------------===//


#ifndef LLVM_LIB_TARGET_AMDGPU_UTILS_AMDGPUMCUTILS_H
#define LLVM_LIB_TARGET_AMDGPU_UTILS_AMDGPUMCUTILS_H

#include "AMDGPU.h"
#include "SIInstrInfo.h"

namespace llvm {
namespace AMDGPU {
  Optional<int64_t> foldToImm(const MachineOperand &Op,
                              const MachineRegisterInfo *MRI, const SIInstrInfo *TII);

} // namespace AMDGPU
} // namespace llvm

#endif // LLVM_LIB_TARGET_AMDGPU_UTILS_AMDGPUMCUTILS_H
