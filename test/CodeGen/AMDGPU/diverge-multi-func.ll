; RUN: llc < %s -march=amdgcn -mcpu=gfx810 -mattr=+vgpr-spilling -verify-machineinstrs | FileCheck -check-prefixes=GCN,GFX81,PREGFX9 %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx900 -mattr=+vgpr-spilling -verify-machineinstrs | FileCheck -check-prefixes=GCN,GFX9 %s

; Testing for an issue in divergence calculation when multi-block function is not the first function to be processed (stale state)

target datalayout = "e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"
target triple = "amdgcn--amdpal"

@0 = external dso_local addrspace(4) constant [4 x <4 x float>]

; GCN-LABEL: {{^}}_amdgpu_ps_main:
define dllexport amdgpu_ps void @_amdgpu_ps_main(i32 inreg %arg, <2 x float> %arg1) local_unnamed_addr #0 {
.entry:
  %tmp = extractelement <2 x float> %arg1, i32 0
  %tmp2 = extractelement <2 x float> %arg1, i32 1
  %tmp3 = call float @llvm.amdgcn.interp.p1(float %tmp, i32 0, i32 0, i32 %arg) #5
  %tmp4 = call float @llvm.amdgcn.interp.p2(float %tmp3, float %tmp2, i32 0, i32 0, i32 %arg) #5
  %tmp5 = call <2 x half> @llvm.amdgcn.cvt.pkrtz(float %tmp4, float undef) #5
  call void @llvm.amdgcn.exp.compr.v2f16(i32 0, i32 15, <2 x half> %tmp5, <2 x half> %tmp5, i1 true, i1 true) #3
  ret void
}

; GCN-LABEL: {{^}}_amdgpu_gs_main:
; GCN-NOT: v_readfirstlane
; PRE-GFX9: flat_load_dword
; GFX9: global_load 
define dllexport amdgpu_gs void @_amdgpu_gs_main(i32 inreg %arg, <4 x i32> inreg %rd, <4 x i32> inreg %rd2, i32 %offset) local_unnamed_addr #1 {
.entry:
  %tmp = call float @llvm.amdgcn.buffer.load.f32(<4 x i32> %rd, i32 0, i32 %offset, i1 true, i1 true) #0
  %tmp1 = fptosi float %tmp to i32
  %t2.shl = shl i32 %tmp1, 2
  %t2.sb.load = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %rd2, i32 %t2.shl, i1 false) #0
  %tmp2 = sext i32 %tmp1 to i64
  %tmp3 = getelementptr [4 x <4 x float>], [4 x <4 x float>] addrspace(4)* @0, i64 0, i64 %tmp2
  %tmp4 = load <4 x float>, <4 x float> addrspace(4)* %tmp3, align 16
  %t.cond = icmp sgt i32 %t2.sb.load, 1
  br i1 %t.cond, label %.lr.ph, label %._crit_edge

.lr.ph:                                           ; preds = %.entry
  %bc8 = bitcast <4 x float> %tmp4 to <4 x i32>
  %tmp5 = extractelement <4 x i32> %bc8, i32 2
  br label %bb

bb:                                               ; preds = %bb, %.lr.ph
  call void @llvm.amdgcn.tbuffer.store.i32(i32 %tmp5, <4 x i32> undef, i32 0, i32 undef, i32 %arg, i32 0, i32 4, i32 4, i1 true, i1 true) #3
  br label %bb

._crit_edge:                                      ; preds = %.entry
  ret void
}

declare float @llvm.amdgcn.interp.p1(float, i32, i32, i32) #1
declare float @llvm.amdgcn.interp.p2(float, float, i32, i32, i32) #1
declare <2 x half> @llvm.amdgcn.cvt.pkrtz(float, float) #1
declare void @llvm.amdgcn.exp.compr.v2f16(i32, i32, <2 x half>, <2 x half>, i1, i1) #0
declare float @llvm.amdgcn.buffer.load.f32(<4 x i32>, i32, i32, i1, i1) #0
declare void @llvm.amdgcn.tbuffer.store.i32(i32, <4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1) #3
declare i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32>, i32, i1) #4

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone speculatable }
attributes #3 = { nounwind writeonly }
attributes #4 = { nounwind readnone }
