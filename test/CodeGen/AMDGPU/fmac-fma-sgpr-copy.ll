; RUN: llc < %s -march=amdgcn -mcpu=gfx1010 -verify-machineinstrs | FileCheck %s

; CHECK: v_fma_f32 v0, v1, v0, s0
define amdgpu_cs float @test1(<4 x i32> inreg %a, float %b, float %y) {
entry:
  %buf.load = call <4 x i32> @llvm.amdgcn.s.buffer.load.v4i32(<4 x i32> %a, i32 0, i32 0)
  %vec1 = bitcast <4 x i32> %buf.load to <4 x float>
  %.i095 = extractelement <4 x float> %vec1, i32 0
  %.i098 = fsub nnan arcp float %b, %.i095
  %fma1 = call float @llvm.fma.f32(float %y, float %.i098, float %.i095) #3
  ret float %fma1
}

declare <4 x i32> @llvm.amdgcn.s.buffer.load.v4i32(<4 x i32>, i32, i32 immarg) #2
declare float @llvm.fma.f32(float, float, float) #1

attributes #1 = { nounwind readnone speculatable willreturn }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }
