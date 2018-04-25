; RUN: llc -march=amdgcn -mcpu=fiji -verify-machineinstrs < %s | FileCheck -check-prefixes=GCN,VI %s
; RUN: llc -march=amdgcn -mcpu=gfx900 -verify-machineinstrs < %s | FileCheck -check-prefixes=GCN,GFX9 %s

; GCN-LABEL: {{^}}test_waterfall_readlane:
; GCN: {{^}}BB0_1:
; GCN: v_readfirstlane_b32 [[VAL1:s[0-9]+]], [[VAL2:v[0-9]+]]
; GCN: v_cmp_eq_u32_e64 [[EXEC:s[[0-9]+:[0-9]+]]], [[VAL1]], [[VAL2]]
; GCN: s_and_saveexec_b64 [[EXEC]], [[EXEC]]
; GCN: v_readlane_b32 [[RLVAL:s[0-9]+]], v1, [[VAL1]]
; GCN: v_mov_b32_e32 [[VVAL:v[0-9]+]], [[RLVAL]]
; GCN: v_or_b32_e32 [[ACCUM:v[0-9]+]], [[ACCUM]], [[VVAL]]
; GCN: s_xor_b64 exec, exec, [[EXEC]]
; GCN: s_cbranch_execnz BB0_1
; GCN: s_mov_b64 exec, s[{{[0-9]+:[0-9]+}}]
; VI: flat_store_dword v[{{[0-9]+:[0-9]+}}], [[ACCUM]]
; GFX9: global_store_dword v[{{[0-9]+:[0-9]+}}], [[ACCUM]], off
define amdgpu_ps void @test_waterfall_readlane(i32 addrspace(1)* inreg %out, <2 x i32> addrspace(1)* inreg %in, i32 %tid, i32 %val) #1 {
  %gep.in = getelementptr <2 x i32>, <2 x i32> addrspace(1)* %in, i32 %tid
  %args = load <2 x i32>, <2 x i32> addrspace(1)* %gep.in
  %value = extractelement <2 x i32> %args, i32 0
  %lane = extractelement <2 x i32> %args, i32 1
  %wf_token = call i32 @llvm.amdgcn.waterfall.begin(i32 %lane)
  %readlane = call i32 @llvm.amdgcn.waterfall.readfirstlane.i32(i32 %wf_token, i32 %lane)
  %readlane1 = call i32 @llvm.amdgcn.readlane(i32 %val, i32 %readlane)
  %readlane2 = call i32 @llvm.amdgcn.waterfall.end.i32(i32 %wf_token, i32 %readlane1)
  ; This store instruction should be outside the waterfall loop and the value
  ; being stored generated incrementally in the loop itself
  store i32 %readlane2, i32 addrspace(1)* %out, align 4

  ret void
}

; GCN-LABEL: {{^}}test_waterfall_non_uniform_img:
; GCN: v_mov_b32_e32 [[IDX:v[0-9]+]], v0
; GCN: v_mov_b32_e32 v[[DSTSTART:[0-9]+]], 0
; GCN: v_mov_b32_e32 v{{[0-9]+}}, 0
; GCN: v_mov_b32_e32 v{{[0-9]+}}, 0
; GCN: v_mov_b32_e32 v[[DSTEND:[0-9]+]], 0
; GCN: s_mov_b64 [[EXEC:s[[0-9]+:[0-9]+]]], exec
; GCN: {{^}}BB1_1:
; GCN: v_readfirstlane_b32 s[[FIRSTVAL:[0-9]+]], [[IDX]]
; GCN: v_cmp_eq_u32_e64 [[EXEC2:s[[0-9]+:[0-9]+]]], s[[FIRSTVAL]], [[IDX]]
; GCN: s_and_saveexec_b64 [[EXEC3:s[[0-9]+:[0-9]+]]], [[EXEC2]]
; GCN: s_load_dwordx8 [[PTR:s\[[0-9]+:[0-9]+\]]], s{{\[}}[[FIRSTVAL]]:{{[0-9]+}}], 0x0
; GCN: s_waitcnt lgkmcnt(0)
; GCN: image_sample v{{\[}}[[VALSTART:[0-9]+]]:[[VALEND:[0-9]+]]{{\]}}, v[{{[0-9]+:[0-9]+}}], [[PTR]], s[{{[0-9]+:[0-9]+}}] dmask:0xf
; GCN: v_or_b32_e32 v[[DSTSTART]], v[[DSTSTART]], v[[VALSTART]]
; GCN: v_or_b32_e32 v{{[0-9]+}}, v{{[0-9]+}}, v{{[0-9]+}}
; GCN: v_or_b32_e32 v{{[0-9]+}}, v{{[0-9]+}}, v{{[0-9]+}}
; GCN: v_or_b32_e32 v[[DSTEND]], v[[DSTEND]], v[[VALEND]]
; GCN: s_xor_b64 exec, exec, [[EXEC3]]
; GCN: s_cbranch_execnz BB1_1
; GCN: s_and_b64 exec, exec, s[{{[0-9]+:[0-9]+}}]
; GCN: s_mov_b64 exec, [[EXEC]]
define amdgpu_ps <4 x float> @test_waterfall_non_uniform_img(<8 x i32> addrspace(4)* inreg %in, i32 %index) #1 {
  %wf_token = call i32 @llvm.amdgcn.waterfall.begin(i32 %index)
  %s_idx = call i32 @llvm.amdgcn.waterfall.readfirstlane.i32(i32 %wf_token, i32 %index)
  %ptr = getelementptr <8 x i32>, <8 x i32> addrspace(4)* %in, i32 %s_idx
  %rsrc = load <8 x i32>, <8 x i32> addrspace(4) * %ptr, align 32
  %r = call <4 x float> @llvm.amdgcn.image.sample.v4f32.v4f32.v8i32(<4 x float> undef, <8 x i32> %rsrc, <4 x i32> undef, i32 15, i1 0, i1 0, i1 0, i1 0, i1 0)
  %r1 = call <4 x float> @llvm.amdgcn.waterfall.end.v4f32(i32 %wf_token, <4 x float> %r)

  ret <4 x float> %r1
}

; GCN-LABEL: {{^}}test_waterfall_non_uniform_img_single_read:
; VI: flat_load_dwordx4 v{{\[}}[[RSRCSTART:[0-9]+]]:{{[0-9]+}}], v[{{[0-9]+:[0-9]+}}]
; VI: flat_load_dwordx4 v[{{[0-9]+:}}[[RSRCEND:[0-9]+]]{{\]}}, v[{{[0-9]+:[0-9]+}}]
; GFX9-DAG: global_load_dwordx4 v{{\[}}[[RSRCSTART:[0-9]+]]:{{[0-9]+}}], v[{{[0-9]+:[0-9]+}}], off{{$}}
; GFX9-DAG: global_load_dwordx4 v[{{[0-9]+:}}[[RSRCEND:[0-9]+]]{{\]}}, v[{{[0-9]+:[0-9]+}}], off offset:16
; GCN: v_mov_b32_e32 v[[DSTSTART:[0-9]+]], 0
; GCN: v_mov_b32_e32 v{{[0-9]+}}, 0
; GCN: v_mov_b32_e32 v{{[0-9]+}}, 0
; GCN-DAG: v_mov_b32_e32 v[[DSTEND:[0-9]+]], 0
; GCN-DAG: s_mov_b64 [[EXEC:s[[0-9]+:[0-9]+]]], exec
; GCN: {{^}}BB2_1:
; GCN: v_readfirstlane_b32 s[[FIRSTVAL:[0-9]+]], v0
; GCN: v_cmp_eq_u32_e64 [[EXEC2:s[[0-9]+:[0-9]+]]], s[[FIRSTVAL]], v0
; GCN-DAG: v_readfirstlane_b32 s[[FIRSTRSRC:[0-9]+]], v[[RSRCSTART]]
; GCN-DAG: v_readfirstlane_b32 s[[ENDRSRC:[0-9]+]], v[[RSRCEND]]
; GCN-DAG: v_readfirstlane_b32 s{{[0-9]+}}, v{{[0-9]+}}
; GCN-DAG: v_readfirstlane_b32 s{{[0-9]+}}, v{{[0-9]+}}
; GCN-DAG: v_readfirstlane_b32 s{{[0-9]+}}, v{{[0-9]+}}
; GCN-DAG: v_readfirstlane_b32 s{{[0-9]+}}, v{{[0-9]+}}
; GCN-DAG: v_readfirstlane_b32 s{{[0-9]+}}, v{{[0-9]+}}
; GCN-DAG: v_readfirstlane_b32 s{{[0-9]+}}, v{{[0-9]+}}
; GCN: s_and_saveexec_b64 [[EXEC3:s[[0-9]+:[0-9]+]]], [[EXEC2]]
; GCN: image_sample v{{\[}}[[VALSTART:[0-9]+]]:[[VALEND:[0-9]+]]{{\]}}, v[{{[0-9]+:[0-9]+}}], s{{\[}}[[FIRSTRSRC]]:[[ENDRSRC]]{{\]}}, s[{{[0-9]+:[0-9]+}}] dmask:0xf
; GCN: v_or_b32_e32 v[[DSTSTART]], v[[DSTSTART]], v[[VALSTART]]
; GCN: v_or_b32_e32 v{{[0-9]+}}, v{{[0-9]+}}, v{{[0-9]+}}
; GCN: v_or_b32_e32 v{{[0-9]+}}, v{{[0-9]+}}, v{{[0-9]+}}
; GCN: v_or_b32_e32 v[[DSTEND]], v[[DSTEND]], v[[VALEND]]
; GCN: s_xor_b64 exec, exec, [[EXEC3]]
; GCN: s_cbranch_execnz BB2_1
; GCN: s_and_b64 exec, exec, s[{{[0-9]+:[0-9]+}}]
; GCN: s_mov_b64 exec, [[EXEC]]
; GCN: v_mov_b32_e32 v0, v[[DSTSTART]]
; GCN: v_mov_b32_e32 v1, v{{[0-9]+}}
; GCN: v_mov_b32_e32 v2, v{{[0-9]+}}
; GCN: v_mov_b32_e32 v3, v[[DSTEND]]
define amdgpu_ps <4 x float> @test_waterfall_non_uniform_img_single_read(<8 x i32> addrspace(4)* inreg %in, i32 %index) #1 {
  %ptr = getelementptr <8 x i32>, <8 x i32> addrspace(4)* %in, i32 %index
  %rsrc = load <8 x i32>, <8 x i32> addrspace(4) * %ptr, align 32
  %wf_token = call i32 @llvm.amdgcn.waterfall.begin(i32 %index)
  %s_rsrc = call <8 x i32> @llvm.amdgcn.waterfall.readfirstlane.v8i32(i32 %wf_token, <8 x i32> %rsrc)
  %r = call <4 x float> @llvm.amdgcn.image.sample.v4f32.v4f32.v8i32(<4 x float> undef, <8 x i32> %s_rsrc, <4 x i32> undef, i32 15, i1 0, i1 0, i1 0, i1 0, i1 0)
  %r1 = call <4 x float> @llvm.amdgcn.waterfall.end.v4f32(i32 %wf_token, <4 x float> %r)

  ret <4 x float> %r1
}

declare i32 @llvm.amdgcn.waterfall.begin(i32) #0
declare i32 @llvm.amdgcn.waterfall.readfirstlane.i32(i32, i32) #0
declare <8 x i32> @llvm.amdgcn.waterfall.readfirstlane.v8i32(i32, <8 x i32>) #0
declare i32 @llvm.amdgcn.waterfall.end.i32(i32, i32) #0
declare <4 x float> @llvm.amdgcn.waterfall.end.v4f32(i32, <4 x float>) #0
declare i32 @llvm.amdgcn.readlane(i32, i32) #0
declare <4 x float> @llvm.amdgcn.image.sample.v4f32.v4f32.v8i32(<4 x float>, <8 x i32>, <4 x i32>, i32, i1, i1, i1, i1, i1)

attributes #0 = { nounwind readnone convergent }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone }
