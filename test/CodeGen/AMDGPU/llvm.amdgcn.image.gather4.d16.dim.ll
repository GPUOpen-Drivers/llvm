; RUN: llc < %s -march=amdgcn -mcpu=tonga -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=UNPACKED %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx810 -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=PACKED -check-prefix=GFX81 %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx900 -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=PACKED -check-prefix=GFX9 %s

; GCN-LABEL: {{^}}image_gather4_b_2d_v4f16:
; UNPACKED: image_gather4_b v[0:3], v[0:3], s[0:7], s[8:11] dmask:0x4 d16{{$}}
; PACKED: image_gather4_b v[0:1], v[0:3], s[0:7], s[8:11] dmask:0x4 d16{{$}}
define amdgpu_ps <2 x float> @image_gather4_b_2d_v4f16(<8 x i32> inreg %rsrc, <4 x i32> inreg %samp, float %bias, float %s, float %t) {
main_body:
  %tex = call <4 x half> @llvm.amdgcn.image.gather4.b.2d.v4f16.f32.f32(i32 4, float %bias, float %s, float %t, <8 x i32> %rsrc, <4 x i32> %samp, i1 false, i32 0, i32 0)
  %r = bitcast <4 x half> %tex to <2 x float>
  ret <2 x float> %r
}

; GCN-LABEL: {{^}}image_gather4_b_2d_v4f16_tfe:
; GCN: v_mov_b32_e32 v{{[0-9]+}}, 0
; UNPACKED: image_gather4_b v[{{[0-9]+:[0-9]+}}], v[0:3], s[0:7], s[8:11] dmask:0x4 tfe d16{{$}}
; PACKED: image_gather4_b v[{{[0-9]+:[0-9]+}}], v[0:3], s[0:7], s[8:11] dmask:0x4 tfe d16{{$}}
define amdgpu_ps <4 x float> @image_gather4_b_2d_v4f16_tfe(<8 x i32> inreg %rsrc, <4 x i32> inreg %samp, float %bias, float %s, float %t) {
main_body:
  %tex = call <8 x half> @llvm.amdgcn.image.gather4.b.2d.v8f16.f32.f32(i32 4, float %bias, float %s, float %t, <8 x i32> %rsrc, <4 x i32> %samp, i1 false, i32 1, i32 0)
  %r = bitcast <8 x half> %tex to <4 x float>
  ret <4 x float> %r
}

declare <4 x half> @llvm.amdgcn.image.gather4.b.2d.v4f16.f32.f32(i32, float, float, float, <8 x i32>, <4 x i32>, i1, i32, i32) #1
declare <8 x half> @llvm.amdgcn.image.gather4.b.2d.v8f16.f32.f32(i32, float, float, float, <8 x i32>, <4 x i32>, i1, i32, i32) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind readnone }
