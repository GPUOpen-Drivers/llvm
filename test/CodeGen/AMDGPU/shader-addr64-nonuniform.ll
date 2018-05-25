; RUN: llc -mtriple=amdgcn--amdpal -mcpu=gfx700 -verify-machineinstrs <%s | FileCheck -enable-var-scope -check-prefix=GCN -check-prefix=SICI %s

; GCN-LABEL: {{^}}main:
; GCN-NOT: readfirstlane
; SICI: buffer_load_dwordx4 {{.*}} addr64

@indexable = internal unnamed_addr addrspace(1) constant [6 x <3 x float>] [<3 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00>, <3 x float> <float 0.000000e+00, float 1.000000e+00, float 0.000000e+00>, <3 x float> <float 0.000000e+00, float 0.000000e+00, float 1.000000e+00>, <3 x float> <float 0.000000e+00, float 1.000000e+00, float 1.000000e+00>, <3 x float> <float 1.000000e+00, float 0.000000e+00, float 1.000000e+00>, <3 x float> <float 1.000000e+00, float 1.000000e+00, float 0.000000e+00>]

define amdgpu_ps float @main(i32 %arg18) {
.entry:
  %tmp31 = sext i32 %arg18 to i64
  %tmp32 = getelementptr [6 x <3 x float>], [6 x <3 x float>] addrspace(1)* @indexable, i64 0, i64 %tmp31
  %tmp33 = load <3 x float>, <3 x float> addrspace(1)* %tmp32, align 16
  %tmp34 = extractelement <3 x float> %tmp33, i32 0
  ret float %tmp34
}

