; RUN: llc < %s -march=amdgcn -mcpu=verde -verify-machineinstrs | FileCheck %s
; RUN: llc < %s -march=amdgcn -mcpu=tonga -verify-machineinstrs | FileCheck %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx900 -verify-machineinstrs | FileCheck %s
;CHECK: s_add_i32 s0, s0, s2
;CHECK: s_add_i32 s0, s0, s3
;CHECK: s_add_i32 s0, s0, s4
;CHECK: s_add_i32 s0, s0, s5
;CHECK: s_add_i32 s0, s0, s6
;CHECK: s_add_i32 s0, s0, s7
;CHECK: s_add_i32 s0, s0, s8
;CHECK: s_add_i32 s0, s0, s9
;CHECK: s_add_i32 s0, s0, s10
;CHECK: s_add_i32 s0, s0, s11
;CHECK: s_add_i32 s0, s0, s12
;CHECK: s_add_i32 s0, s0, s13
;CHECK: s_add_i32 s0, s0, s14
;CHECK: s_add_i32 s0, s0, s15
;CHECK: s_add_i32 s0, s0, s16
;CHECK: s_add_i32 s0, s0, s17
;CHECK: s_add_i32 s0, s0, s18
;CHECK: s_add_i32 s0, s0, s19
;CHECK: s_add_i32 s0, s0, s20
;CHECK: s_add_i32 s0, s0, s21
;CHECK: s_add_i32 s0, s0, s22
;CHECK: s_add_i32 s0, s0, s23
;CHECK: s_add_i32 s0, s0, s24
;CHECK: s_add_i32 s0, s0, s25
;CHECK: s_add_i32 s0, s0, s26
;CHECK: s_add_i32 s0, s0, s27
;CHECK: s_add_i32 s0, s0, s28
;CHECK: s_add_i32 s0, s0, s29
;CHECK: s_add_i32 s0, s0, s30
;CHECK: s_add_i32 s0, s0, s31
;CHECK: s_add_i32 s0, s0, s32
;CHECK: s_add_i32 s0, s0, s33
;CHECK: s_add_i32 s0, s0, s34
;CHECK: s_add_i32 s0, s0, s35
;CHECK: s_add_i32 s0, s0, s36
;CHECK: s_add_i32 s0, s0, s37
;CHECK: s_add_i32 s0, s0, s38
;CHECK: s_add_i32 s0, s0, s39
;CHECK: s_add_i32 s0, s0, s40
;CHECK: s_add_i32 s0, s0, s41
;CHECK: s_add_i32 s0, s0, s42
;CHECK: s_add_i32 s0, s0, s43
;CHECK: s_add_i32 s0, s0, s44
;CHECK: s_add_i32 s0, s0, s45
;CHECK: s_add_i32 s0, s0, s46
;CHECK: s_add_i32 s0, s0, s47
;CHECK: s_add_i32 s0, s0, s48
;CHECK: s_add_i32 s0, s0, s49
;CHECK: s_add_i32 s0, s0, s50
;CHECK: s_add_i32 s0, s0, s51
;CHECK: s_add_i32 s0, s0, s52
;CHECK: s_add_i32 s0, s0, s53
;CHECK: s_add_i32 s0, s0, s54
;CHECK: v_mov_b32_e32 v0, s0
;CHECK: buffer_store_short v0, off, s[56:59], 0 offset:16
;CHECK: s_endpgm
define amdgpu_gs void @_amdgpu_gs_main(i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, <4 x i32> inreg) {
.entry:
  %56 = add i32 %0, %1
  %57 =  add i32 %56, %2
  %58 =  add i32 %57, %3
  %59 =  add i32 %58, %4
  %60 =  add i32 %59, %5
  %61 =  add i32 %60, %6
  %62 =  add i32 %61, %7
  %63 =  add i32 %62, %8
  %64 =  add i32 %63, %9
  %65 =  add i32 %64, %10
  %66 =  add i32 %65, %11
  %67 =  add i32 %66, %12
  %68 =  add i32 %67, %13
  %69 =  add i32 %68, %14
  %70 =  add i32 %69, %15
  %71 =  add i32 %70, %16
  %72 =  add i32 %71, %17
  %73 =  add i32 %72, %18
  %74 =  add i32 %73, %19
  %75 =  add i32 %74, %20
  %76 =  add i32 %75, %21
  %77 =  add i32 %76, %22
  %78 =  add i32 %77, %23
  %79 =  add i32 %78, %24
  %80 =  add i32 %79, %25
  %81 =  add i32 %80, %26
  %82 =  add i32 %81, %27
  %83 =  add i32 %82, %28
  %84 =  add i32 %83, %29
  %85 =  add i32 %84, %30
  %86 =  add i32 %85, %31
  %87 =  add i32 %86, %32
  %88 =  add i32 %87, %33
  %89 =  add i32 %88, %34
  %90 =  add i32 %89, %35
  %91 =  add i32 %90, %36
  %92 =  add i32 %91, %37
  %93 =  add i32 %92, %38
  %94 =  add i32 %93, %39
  %95 =  add i32 %94, %40
  %96 =  add i32 %95, %41
  %97 =  add i32 %96, %42
  %98 =  add i32 %97, %43
  %99 =  add i32 %98, %44
  %100 =  add i32 %99, %45
  %101 =  add i32 %100, %46
  %102 =  add i32 %101, %47
  %103 =  add i32 %102, %48
  %104 =  add i32 %103, %49
  %105 =  add i32 %104, %50
  %106 =  add i32 %105, %51
  %107 =  add i32 %106, %52
  %108 =  add i32 %107, %53
  %109 =  add i32 %108, %54
  %res = trunc i32 %109 to i16
  call void @llvm.amdgcn.buffer.store.i16(i16 %res, <4 x i32> %55, i32 0, i32 16, i1 0, i1 0)
  ret void
}

declare void @llvm.amdgcn.buffer.store.i16(i16, <4 x i32>, i32, i32, i1, i1) #0
