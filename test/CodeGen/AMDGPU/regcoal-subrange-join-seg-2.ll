; RUN: llc -march=amdgcn -stop-after simple-register-coalescing -o - %s | FileCheck --check-prefix=GCN %s
;
; See bug http://llvm.org/PR35374 for details of the problem being checked here
; This test will provoke a subrange join failure without a fix for PR35374
; As such we only test for successful simple-register-coalescing
;
; GCN: S_CBRANCH_SCC1 %bb.6{{.*}}

; Function Attrs: nounwind
define amdgpu_cs void @main() local_unnamed_addr #0 {
.entry:
  br i1 undef, label %._crit_edge, label %.lr.ph

.lr.ph:                                           ; preds = %.entry
  br label %bb

bb:                                               ; preds = %bb23, %.lr.ph
  br label %bb1

bb1:                                              ; preds = %bb1, %bb
  %__llpc_global_proxy_r9.11793 = phi <4 x i32> [ undef, %bb ], [ %tmp, %bb1 ]
  %tmp = shufflevector <4 x i32> %__llpc_global_proxy_r9.11793, <4 x i32> undef, <4 x i32> <i32 4, i32 5, i32 2, i32 6>
  br i1 undef, label %bb2, label %bb1

bb2:                                              ; preds = %bb1
  %tmp3 = shufflevector <4 x i32> undef, <4 x i32> %tmp, <4 x i32> <i32 0, i32 1, i32 6, i32 undef>
  %__llpc_global_proxy_r9.0.vec.insert = shufflevector <4 x i32> %tmp3, <4 x i32> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 6>
  br label %bb4

bb4:                                              ; preds = %bb4, %bb2
  %__llpc_global_proxy_r5.12.vec.insert9681805 = phi <4 x i32> [ undef, %bb2 ], [ %__llpc_global_proxy_r5.12.vec.insert968, %bb4 ]
  %__llpc_global_proxy_r9.21801 = phi <4 x i32> [ %__llpc_global_proxy_r9.0.vec.insert, %bb2 ], [ undef, %bb4 ]
  %__llpc_global_proxy_r7.31800 = phi <4 x i32> [ undef, %bb2 ], [ %tmp21, %bb4 ]
  %tmp5 = shufflevector <4 x i32> %__llpc_global_proxy_r9.21801, <4 x i32> undef, <2 x i32> zeroinitializer
  %tmp6 = add <2 x i32> %tmp5, <i32 1, i32 6>
  %tmp7 = load float, float* undef, align 16
  %tmp8 = fmul float undef, %tmp7
  %tmp9 = fadd float undef, %tmp8
  %tmp10 = fadd float %tmp9, undef
  %tmp11 = bitcast float %tmp10 to i32
  %__llpc_global_proxy_r5.12.vec.insert996 = insertelement <4 x i32> %__llpc_global_proxy_r5.12.vec.insert9681805, i32 %tmp11, i32 3
  %tmp12 = insertelement <3 x i32> undef, i32 %tmp11, i32 0
  %tmp13 = shufflevector <3 x i32> %tmp12, <3 x i32> undef, <3 x i32> zeroinitializer
  %tmp14 = bitcast <3 x i32> %tmp13 to <3 x float>
  %tmp15 = fmul <3 x float> undef, %tmp14
  %tmp16 = shufflevector <4 x i32> %__llpc_global_proxy_r7.31800, <4 x i32> undef, <3 x i32> <i32 0, i32 1, i32 3>
  %tmp17 = bitcast <3 x i32> %tmp16 to <3 x float>
  %tmp18 = fadd <3 x float> %tmp15, %tmp17
  %tmp19 = bitcast <3 x float> %tmp18 to <3 x i32>
  %tmp20 = shufflevector <3 x i32> %tmp19, <3 x i32> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 undef>
  %tmp21 = shufflevector <4 x i32> %__llpc_global_proxy_r7.31800, <4 x i32> %tmp20, <4 x i32> <i32 4, i32 5, i32 2, i32 6>
  %__llpc_global_proxy_r9.0.vec.extract1342 = extractelement <2 x i32> %tmp6, i32 0
  %tmp22 = icmp sgt i32 %__llpc_global_proxy_r9.0.vec.extract1342, 5
  %__llpc_global_proxy_r5.12.vec.insert968 = insertelement <4 x i32> %__llpc_global_proxy_r5.12.vec.insert996, i32 undef, i32 3
  br i1 %tmp22, label %bb23, label %bb4

bb23:                                             ; preds = %bb4
  %__llpc_global_proxy_r5.4.vec.extract = extractelement <4 x i32> %__llpc_global_proxy_r5.12.vec.insert9681805, i32 1
  %tmp24 = icmp ult i32 0, %__llpc_global_proxy_r5.4.vec.extract
  br i1 %tmp24, label %bb, label %._crit_edge

._crit_edge:                                      ; preds = %bb23, %.entry
  ret void
}
