; RUN: llc -march=amdgcn -mcpu=gfx900 -verify-machineinstrs < %s | FileCheck -check-prefixes=GFX9_32BANK %s
; RUN: llc -march=amdgcn -mcpu=fiji -verify-machineinstrs < %s | FileCheck -check-prefixes=GFX8_32BANK %s
; RUN: llc -march=amdgcn -mcpu=gfx810 -verify-machineinstrs < %s | FileCheck -check-prefixes=GFX8_16BANK %s

; GFX9_32BANK-LABEL: {{^}}interp_f16:
; GFX9_32BANK: s_mov_b32 m0, s{{[0-9]+}}
; GFX9_32BANK: v_interp_p1ll_f16{{(_e32)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y{{$}}
; GFX9_32BANK: v_interp_p1ll_f16{{(_e32)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y high
; GFX9_32BANK: v_interp_p2_legacy_f16{{(_e32)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y, v{{[0-9]*}}{{$}}
; GFX9_32BANK: v_interp_p2_legacy_f16{{(_e32)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y, v{{[0-9]*}} high

; GFX8_32BANK-LABEL: {{^}}interp_f16:
; GFX8_32BANK: s_mov_b32 m0, s{{[0-9]+}}
; GFX8_32BANK: v_interp_p1ll_f16{{(_e32)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y{{$}}
; GFX8_32BANK: v_interp_p1ll_f16{{(_e32)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y high
; GFX8_32BANK: v_interp_p2_f16{{(_e32)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y, v{{[0-9]*}}{{$}}
; GFX8_32BANK: v_interp_p2_f16{{(_e32)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y, v{{[0-9]*}} high

; GFX8_16BANK-LABEL: {{^}}interp_f16:
; GFX8_16BANK: s_mov_b32 m0, s{{[0-9]+}}
; there should be only one v_interp_mov
; GFX8_16BANK: v_interp_mov_f32_e32 v{{[0-9]+}}, p0, attr2.y
; GFX8_16BANK-NOT: v_interp_mov_f32_e32 v{{[0-9]+}}, p0, attr2.y
; GFX8_16BANK: v_interp_p1lv_f16{{(_e64)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y, v{{[0-9]*}}{{$}}
; GFX8_16BANK: v_interp_p1lv_f16{{(_e64)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y, v{{[0-9]*}} high
; GFX8_16BANK: v_interp_p2_f16{{(_e64)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y, v{{[0-9]*}}{{$}}
; GFX8_16BANK: v_interp_p2_f16{{(_e64)*}} v{{[0-9]+}}, v{{[0-9]+}}, attr2.y, v{{[0-9]*}} high

define amdgpu_ps half @interp_f16(float inreg %i, float inreg %j, i32 inreg %m0) #0 {
main_body:
  %p1_0 = call float @llvm.amdgcn.interp.p1.f16(float %i, i32 1, i32 2, i1 0, i32 %m0)
  %p1_1 = call float @llvm.amdgcn.interp.p1.f16(float %i, i32 1, i32 2, i1 1, i32 %m0)
  %p2_0 = call half @llvm.amdgcn.interp.p2.f16(float %p1_0, float %j, i32 1, i32 2, i1 0, i32 %m0)
  %p2_1 = call half @llvm.amdgcn.interp.p2.f16(float %p1_1, float %j, i32 1, i32 2, i1 1, i32 %m0)
  %res = fadd half %p2_0, %p2_1
  ret half %res
}

; float @llvm.amdgcn.interp.p1.f16(i, attrchan, attr, high, m0)
declare float @llvm.amdgcn.interp.p1.f16(float, i32, i32, i1, i32) #0
; half @llvm.amdgcn.interp.p1.f16(p1, j, attrchan, attr, high, m0)
declare half @llvm.amdgcn.interp.p2.f16(float, float, i32, i32, i1, i32) #0
declare float @llvm.amdgcn.interp.mov(i32, i32, i32, i32) #0

attributes #0 = { nounwind readnone }
