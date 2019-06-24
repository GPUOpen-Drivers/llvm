; Modifications Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
; Notified per clause 4(b) of the license.
; RUN: llc -march=amdgcn -mcpu=gfx900 <%s | FileCheck -check-prefix=GCN %s

; Ensure that all the raw and struct atomic intrinsics are a source of divergence.

; GCN-LABEL: raw_buffer_atomic_swap:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_swap(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.swap.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_add:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_add(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.add.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_sub:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_sub(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.sub.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_smin:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_smin(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.smin.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_umin:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_umin(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.umin.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_smax:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_smax(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.smax.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_umax:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_umax(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.umax.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_and:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_and(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.and.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_or:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_or(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.or.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_xor:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_xor(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.xor.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: raw_buffer_atomic_cmpswap:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @raw_buffer_atomic_cmpswap(i32 inreg %val, i32 inreg %cmp, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.raw.buffer.atomic.cmpswap.i32(i32 %val, i32 %cmp, <4 x i32> %desc, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_swap:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_swap(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.swap.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_add:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_add(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.add.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_sub:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_sub(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.sub.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_smin:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_smin(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.smin.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_umin:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_umin(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.umin.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_smax:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_smax(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.smax.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_umax:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_umax(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.umax.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_and:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_and(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.and.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_or:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_or(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.or.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_xor:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_xor(i32 inreg %val, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.xor.i32(i32 %val, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

; GCN-LABEL: struct_buffer_atomic_cmpswap:
; GCN: buffer_load_dword v{{[0-9]+}},
define amdgpu_ps void @struct_buffer_atomic_cmpswap(i32 inreg %val, i32 inreg %cmp, <4 x i32> inreg %desc, i32 addrspace(4)* inreg %ptr) {
  %res = call i32 @llvm.amdgcn.struct.buffer.atomic.cmpswap.i32(i32 %val, i32 %cmp, <4 x i32> %desc, i32 0, i32 0, i32 0, i32 0)
  %load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %desc, i32 %res, i32 0)
  store i32 %load, i32 addrspace(5)* undef
  ret void
}

declare i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32>, i32, i32)
declare i32 @llvm.amdgcn.raw.buffer.atomic.swap.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.add.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.sub.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.smin.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.umin.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.smax.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.umax.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.and.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.or.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.xor.i32(i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.raw.buffer.atomic.cmpswap.i32(i32, i32, <4 x i32>, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.swap.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.add.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.sub.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.smin.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.umin.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.smax.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.umax.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.and.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.or.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.xor.i32(i32, <4 x i32>, i32, i32, i32, i32 immarg)
declare i32 @llvm.amdgcn.struct.buffer.atomic.cmpswap.i32(i32, i32, <4 x i32>, i32, i32, i32, i32 immarg)

