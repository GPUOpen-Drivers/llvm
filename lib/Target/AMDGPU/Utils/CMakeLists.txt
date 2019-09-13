# Modifications Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
# Notified per clause 4(b) of the license.
add_llvm_library(LLVMAMDGPUUtils
  AMDGPUBaseInfo.cpp
  AMDKernelCodeTUtils.cpp
  AMDGPUAsmUtils.cpp
  AMDGPUPALMetadata.cpp
  AMDGPUMCUtils.cpp
  )
