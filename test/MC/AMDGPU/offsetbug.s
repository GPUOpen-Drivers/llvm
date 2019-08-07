// Modifications Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
// Notified per clause 4(b) of the license.
// RUN: llvm-mc -arch=amdgcn -mcpu=gfx1010 -show-encoding %s | FileCheck %s --check-prefix=GFX10
// RUN: llvm-mc -arch=amdgcn -mcpu=gfx1010 -filetype=obj %s | llvm-objdump -disassemble -mcpu=gfx1010 - | FileCheck %s --check-prefix=BIN
	s_getpc_b64 s[0:1]
	v_add_nc_u32_e32 v4, s6, v0
	s_mov_b64 s[16:17], s[0:1]
	s_mov_b64 s[18:19], s[0:1]
	s_mov_b64 s[24:25], s[0:1]
	s_mov_b32 s0, s5
	s_mov_b32 s18, s4
	s_mov_b32 s16, s3
	s_mov_b32 s24, s2
	s_load_dwordx4 s[8:11], s[0:1], 0x10
	s_load_dwordx4 s[12:15], s[0:1], 0x0
	s_load_dwordx4 s[4:7], s[18:19], 0x0
	s_load_dwordx4 s[20:23], s[16:17], 0x0
	s_load_dwordx4 s[0:3], s[24:25], 0x0
	s_waitcnt lgkmcnt(0)
	tbuffer_load_format_x v0, v4, s[8:11],  format:22, 0 idxen offset:4
	tbuffer_load_format_xyzw v[9:12], v4, s[8:11],  format:56, 0 idxen offset:8
	tbuffer_load_format_xyzw v[13:16], v4, s[8:11],  format:56, 0 idxen offset:12
	s_waitcnt vmcnt(1)
	s_cbranch_vccnz BB0_2
// GFX10: s_cbranch_vccnz BB0_2           ; encoding: [A,A,0x87,0xbf]
// GFX10-NEXT: ;   fixup A - offset: 0, value: BB0_2, kind: fixup_si_sopp_br
// BIN: s_cbranch_vccnz BB0_2 // 00000000006C: BF870060
	tbuffer_load_format_xyzw v[8:11], v4, s[8:11],  format:56, 0 idxen offset:16
	tbuffer_load_format_x v1, v4, s[8:11],  format:22, 0 idxen offset:20
	tbuffer_load_format_x v2, v4, s[8:11],  format:22, 0 idxen offset:24
	tbuffer_load_format_x v3, v4, s[8:11],  format:22, 0 idxen
	tbuffer_load_format_xyzw v[4:7], v4, s[12:15],  format:74, 0 idxen
	s_buffer_load_dword s62, s[4:7], 0x0
	v_nop
	s_buffer_load_dwordx8 s[12:19], s[20:23], 0x0
	s_buffer_load_dwordx4 s[8:11], s[20:23], 0x20
	s_waitcnt lgkmcnt(0)
	s_and_b64 vcc, exec, s[28:29]
	s_cbranch_vccnz BB0_1
// GFX10: s_cbranch_vccnz BB0_1           ; encoding: [A,A,0x87,0xbf]
// GFX10-NEXT: ;   fixup A - offset: 0, value: BB0_1, kind: fixup_si_sopp_br
// BIN: s_cbranch_vccnz BB0_1 // 0000000000BC: BF870041
	s_nop 0
	s_cbranch_execz BB0_3
// GFX10: s_cbranch_execz BB0_3           ; encoding: [A,A,0x88,0xbf]
// GFX10-NEXT: ;   fixup A - offset: 0, value: BB0_3, kind: fixup_si_sopp_br
// BIN: s_cbranch_execz BB0_3 // 0000000000C8: BF880040
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
        s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_nop 0
	s_buffer_load_dword s26, s[0:3], 0x48
	s_waitcnt lgkmcnt(0)
	s_and_b64 vcc, exec, s[28:29]
BB0_1:
	s_buffer_load_dword s28, s[4:7], 0x10
BB0_3:
	s_waitcnt lgkmcnt(0)
	exp param0 v3, v0, v1, v2
	exp param1 v4, v4, v4, off
	s_cbranch_vccnz BB0_2
// GFX10: s_cbranch_vccnz BB0_2           ; encoding: [A,A,0x87,0xbf]
// GFX10-NEXT: ;   fixup A - offset: 0, value: BB0_2, kind: fixup_si_sopp_br
// BIN: s_cbranch_vccnz BB0_2 // 0000000001E0: BF870003
        s_nop 0
        s_nop 0
        s_nop 0
BB0_2:
        s_nop 0
        s_nop 0
        s_endpgm
