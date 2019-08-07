; Modifications Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
; Notified per clause 4(b) of the license.
; RUN: opt -instcombine -S < %s | FileCheck %s

%foo = type { float, i8 }

define void @func(float* %out, [2 x %foo]* %in) {
  %a = insertelement <2 x [2 x %foo]*> undef, [2 x %foo]* %in, i32 0
  %b = insertelement <2 x [2 x %foo]*> %a, [2 x %foo]* %in, i32 1

; CHECK: %gep = getelementptr [2 x %foo], <2 x [2 x %foo]*> %b, <2 x i64> zeroinitializer, <2 x i64> <i64 0, i64 1>, <2 x i32> zeroinitializer
  %gep = getelementptr [2 x %foo], <2 x [2 x %foo]*> %b, <2 x i32> zeroinitializer, <2 x i32> <i32 0, i32 1>, <2 x i32> zeroinitializer

  %extract = extractelement <2 x float*> %gep, i64 1

  %load = load float, float* %extract, align 4
  store float %load, float* %out
  ret void
}
