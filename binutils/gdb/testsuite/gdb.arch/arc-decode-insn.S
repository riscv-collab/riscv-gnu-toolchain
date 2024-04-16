; This testcase is part of GDB, the GNU debugger.

; Copyright 2017-2024 Free Software Foundation, Inc.

; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

.section .text
.global main

#define TEST_J
#define TEST_JCC
#define TEST_JL
#define TEST_JLCC
#define TEST_B
#define TEST_BBIT
#define TEST_BCC
#define TEST_BI
#define TEST_BL
#define TEST_BRCC
#define TEST_JLI
#define TEST_LEAVE_S
#define TEST_LPCC

; JLI-specific stuff
#ifdef TEST_JLI
jli_table:
    .word 0xdeadbeea
    .word 0xdeadbeea
jli_target:
    .word 0xdeadbeea
    .word 0xdeadbeea

.set jli_offset, 3
#endif
main:

; Each test case requires several symbols to be set, that identify expected
; parameters of this instruction.  Required symbols:
;   ${test}_start: symbol points to start of the test
;   ${test}_end: symbol points to the instruction after the jump/branch
;      instruction.
;   ${test}_target: branch target address.
;   ${test}_has_delay_slot: whether instruction has delay slot.
;   ${test}_cc: condition code numeric value.

    .set r12_value, 0xdead0000
    .set blink_value, 0xdead0004
    .set limm_value, 0xdead0008
    ; Just an integer
    .set r4_value, 0xdead000c
    ; Just an integer
    .set r5_value, 0xdead0010
    ; offset index for BI [c]
    .set r7_value, 4
    .set u6_value, 0x20
    .set s12_target, 0x100

    mov r12, @r12_value
    mov r4, @r4_value
    mov r5, @r5_value
    mov r7, @r7_value
    mov blink, @blink_value
#ifdef TEST_JLI
    ; jli_base aux regnum = 0x290
    sr jli_table, [0x290]
#endif

start_branch_tests:

#ifdef TEST_J

#define TEST_NAME j_c
    ; j [c]
    .set j_c_target, @r4_value
    .set j_c_has_delay_slot,   0
    .set j_c_cc, 0
    j_c_start:
	j [r4]
    j_c_end:

    ; j [blink]
    .set j_blink_target, @blink_value
    .set j_blink_has_delay_slot,   0
    .set j_blink_cc, 0
    mov blink, @j_blink_target
    j_blink_start:
	j [blink]
    j_blink_end:

    ; j limm
    .set j_limm_target, @limm_value
    .set j_limm_has_delay_slot,   0
    .set j_limm_cc, 0
    j_limm_start:
	j @j_limm_target
    j_limm_end:

    ; j u6
    .set j_u6_target, @u6_value
    .set j_u6_has_delay_slot,   0
    .set j_u6_cc, 0
    j_u6_start:
	j @j_u6_target
    j_u6_end:

    ; j s12
    .set j_s12_target, @s12_target
    .set j_s12_has_delay_slot,   0
    .set j_s12_cc, 0
    j_s12_start:
	j @j_s12_target
    j_s12_end:

    ; j.d [c]
    .set j_d_c_target, @r4_value
    .set j_d_c_has_delay_slot,   1
    .set j_d_c_cc, 0
    j_d_c_start:
	j.d [r4]
    j_d_c_end:
	nop_s

    ; j.d [blink]
    .set j_d_blink_target, @blink_value
    .set j_d_blink_has_delay_slot,   1
    .set j_d_blink_cc, 0
    j_d_blink_start:
	j.d [blink]
    j_d_blink_end:
	nop_s

    ; j.d u6
    .set j_d_u6_target, @u6_value
    .set j_d_u6_has_delay_slot,   1
    .set j_d_u6_cc, 0
    j_d_u6_start:
	j.d @j_d_u6_target
    j_d_u6_end:
	nop_s

    ; j.d s12
    .set j_d_s12_target, @s12_target
    .set j_d_s12_has_delay_slot,   1
    .set j_d_s12_cc, 0
    j_d_s12_start:
	j.d @j_d_s12_target
    j_d_s12_end:
	nop_s

    ; j_s [b]
    .set j_s_b_target, @r12_value
    .set j_s_b_has_delay_slot,   0
    .set j_s_b_cc, 0
    j_s_b_start:
	j_s [r12]
    j_s_b_end:

    ; j_s.d [b]
    .set j_s_d_b_target, @r12_value
    .set j_s_d_b_has_delay_slot,   1
    .set j_s_d_b_cc, 0
    j_s_d_b_start:
	j_s.d [r12]
    j_s_d_b_end:
	nop_s

    ; j_s [blink]
    .set j_s_blink_target, @blink_value
    .set j_s_blink_has_delay_slot,   0
    .set j_s_blink_cc, 0
    j_s_blink_start:
	j_s [blink]
    j_s_blink_end:

    ; j_s.d [blink]
    .set j_s_d_blink_target, @blink_value
    .set j_s_d_blink_has_delay_slot,   1
    .set j_c_cc, 0
    j_s_d_blink_start:
	j_s.d [blink]
    j_s_d_blink_end:
	nop_s
#endif /* TEST_J */

#ifdef TEST_JCC
    ; jcc [c]
    .set jcc_c_target, @r4_value
    .set jcc_c_has_delay_slot,   0
    .set jcc_c_cc, 1
    jcc_c_start:
	jeq [r4]
    jcc_c_end:

    ; jcc [blink]
    .set jcc_blink_target, @blink_value
    .set jcc_blink_has_delay_slot,   0
    .set jcc_blink_cc, 2
    jcc_blink_start:
	jnz [blink]
    jcc_blink_end:

    ; jcc limm
    .set jcc_limm_target, @limm_value
    .set jcc_limm_has_delay_slot,   0
    .set jcc_limm_cc, 9
    jcc_limm_start:
	jgt @jcc_limm_target
    jcc_limm_end:

    ; jcc u6
    .set jcc_u6_target, @u6_value
    .set jcc_u6_has_delay_slot,   0
    .set jcc_u6_cc, 0xA
    jcc_u6_start:
	jge @jcc_u6_target
    jcc_u6_end:

    ; jcc.d [c]
    .set jcc_d_c_target, @r4_value
    .set jcc_d_c_has_delay_slot,   1
    .set jcc_d_c_cc, 0xB
    jcc_d_c_start:
	jlt.d [r4]
    jcc_d_c_end:
	nop_s

    ; jcc.d [blink]
    .set jcc_d_blink_target, @blink_value
    .set jcc_d_blink_has_delay_slot,   1
    .set jcc_d_blink_cc, 0xC
    jcc_d_blink_start:
	jle.d [blink]
    jcc_d_blink_end:
	nop_s

    ; jcc.d u6
    .set jcc_d_u6_target, @u6_value
    .set jcc_d_u6_has_delay_slot,   1
    .set jcc_d_u6_cc, 0xE
    jcc_d_u6_start:
	jls.d @jcc_d_u6_target
    jcc_d_u6_end:
	nop_s

    ; jeq_s [blink]
    .set jcc_eq_s_blink_target, @blink_value
    .set jcc_eq_s_blink_has_delay_slot,   0
    .set jcc_eq_s_blink_cc, 1
    jcc_eq_s_blink_start:
	jeq_s [blink]
    jcc_eq_s_blink_end:

    ; jne_s [blink]
    .set jcc_ne_s_blink_target, @blink_value
    .set jcc_ne_s_blink_has_delay_slot,   0
    .set jcc_ne_s_blink_cc, 2
    jcc_ne_s_blink_start:
	jne_s [blink]
    jcc_ne_s_blink_end:
#endif /* TEST_JCC */

#ifdef TEST_JL
    ; jl [c]
    .set jl_c_target, @r4_value
    .set jl_c_has_delay_slot,   0
    .set jl_c_cc, 0
    jl_c_start:
	jl [r4]
    jl_c_end:

    ; jl limm
    .set jl_limm_target, @limm_value
    .set jl_limm_has_delay_slot,   0
    .set jl_limm_cc, 0
    jl_limm_start:
	jl @jl_limm_target
    jl_limm_end:

    ; jl u6
    .set jl_u6_target, @u6_value
    .set jl_u6_has_delay_slot,   0
    .set jl_u6_cc, 0
    jl_u6_start:
	jl @jl_u6_target
    jl_u6_end:

    ; jl s12
    .set jl_s12_target, @s12_target
    .set jl_s12_has_delay_slot,   0
    .set jl_s12_cc, 0
    jl_s12_start:
	jl @jl_s12_target
    jl_s12_end:

    ; jl.d [c]
    .set jl_d_c_target, @r4_value
    .set jl_d_c_has_delay_slot,   1
    .set jl_d_c_cc, 0
    jl_d_c_start:
	jl.d [r4]
    jl_d_c_end:
	nop_s

    ; jl.d u6
    .set jl_d_u6_target, @u6_value
    .set jl_d_u6_has_delay_slot,   1
    .set jl_d_u6_cc, 0
    jl_d_u6_start:
	jl.d @jl_d_u6_target
    jl_d_u6_end:
	nop_s

    ; jl.d s12
    .set jl_d_s12_target, @s12_target
    .set jl_d_s12_has_delay_slot,   1
    .set jl_d_s12_cc, 0
    jl_d_s12_start:
	jl.d @jl_d_s12_target
    jl_d_s12_end:
	nop_s

    ; jl_s [b]
    .set jl_s_b_target, @r12_value
    .set jl_s_b_has_delay_slot,   0
    .set jl_s_b_cc, 0
    jl_s_b_start:
	jl_s [r12]
    jl_s_b_end:

    ; jl_s.d [b]
    .set jl_s_d_b_target, @r12_value
    .set jl_s_d_b_has_delay_slot,   1
    .set jl_s_d_b_cc, 0
    jl_s_d_b_start:
	jl_s.d [r12]
    jl_s_d_b_end:
	nop_s
#endif /* TEST_JL */

#ifdef TEST_JLCC
    ; jlcc [c]
    .set jlcc_c_target, @r4_value
    .set jlcc_c_has_delay_slot,   0
    .set jlcc_c_cc, 1
    jlcc_c_start:
	jleq [r4]
    jlcc_c_end:

    ; jlcc limm
    .set jlcc_limm_target, @limm_value
    .set jlcc_limm_has_delay_slot,   0
    .set jlcc_limm_cc, 0x9
    jlcc_limm_start:
	jlgt @jlcc_limm_target
    jlcc_limm_end:

    ; jlcc u6
    .set jlcc_u6_target, @u6_value
    .set jlcc_u6_has_delay_slot,   0
    .set jlcc_u6_cc, 0xA
    jlcc_u6_start:
	jlge @jlcc_u6_target
    jlcc_u6_end:

    ; jlcc.d [c]
    .set jlcc_d_c_target, @r4_value
    .set jlcc_d_c_has_delay_slot,   1
    .set jlcc_d_c_cc, 0xB
    jlcc_d_c_start:
	jllt.d [r4]
    jlcc_d_c_end:
	nop_s

    ; jlcc.d u6
    .set jlcc_d_u6_target, @u6_value
    .set jlcc_d_u6_has_delay_slot,   1
    .set jlcc_d_u6_cc, 0xE
    jlcc_d_u6_start:
	jlls.d @jlcc_d_u6_target
    jlcc_d_u6_end:
	nop_s
#endif /* TEST_JLCC */

#ifdef TEST_B
.Lb_target:
    ; Artifical nop, so that first b will not branch to itself.
    nop_s
    ; b s25
    .set b_s25_target, @.Lb_target
    .set b_s25_has_delay_slot,   0
    .set b_s25_cc, 0
    b_s25_start:
	b @b_s25_target
    b_s25_end:

    ; b.d s25
    .set b_d_s25_target, @.Lb_target
    .set b_d_s25_has_delay_slot,   1
    .set b_d_s25_cc, 0
    b_d_s25_start:
	b.d @b_d_s25_target
    b_d_s25_end:
	nop_s

    ; b_s s10
    .set b_s_s10_target, @.Lb_target
    .set b_s_s10_has_delay_slot,   0
    .set b_s_s10_cc, 0
    b_s_s10_start:
	b_s @b_s_s10_target
    b_s_s10_end:
#endif /* TEST_B */

#ifdef TEST_BBIT

; Due to specifics of bbit implementation in assembler, only local symbols can
; be used as a branch targets for bbit and brcc.
; bbits and brcc don't have condition code set to anything.
.Lbbit_target:
    nop_s

    ; bbit0.nt b,c,s9
    .set bbit0_nt_b_c_s9_target, @.Lbbit_target
    .set bbit0_nt_b_c_s9_has_delay_slot,   0
    .set bbit0_nt_b_c_s9_cc, 0
    bbit0_nt_b_c_s9_start:
	bbit0.nt r4,r5,@bbit0_nt_b_c_s9_target
    bbit0_nt_b_c_s9_end:

    ; bbit0.d.nt b,c,s9
    .set bbit0_d_nt_b_c_s9_target, @.Lbbit_target
    .set bbit0_d_nt_b_c_s9_has_delay_slot,   1
    .set bbit0_d_nt_b_c_s9_cc, 0
    bbit0_d_nt_b_c_s9_start:
	bbit0.d.nt r4,r5,@.Lbbit_target
    bbit0_d_nt_b_c_s9_end:
	nop_s

    ; bbit0.t b,c,s9
    .set bbit0_t_b_c_s9_target, @.Lbbit_target
    .set bbit0_t_b_c_s9_has_delay_slot,   0
    .set bbit0_t_b_c_s9_cc, 0
    bbit0_t_b_c_s9_start:
	bbit0.t r4,r5,@.Lbbit_target
    bbit0_t_b_c_s9_end:

    ; bbit0.d.t b,c,s9
    .set bbit0_d_t_b_c_s9_target, @.Lbbit_target
    .set bbit0_d_t_b_c_s9_has_delay_slot,   1
    .set bbit0_d_t_b_c_s9_cc, 0
    bbit0_d_t_b_c_s9_start:
	bbit0.d.t r4,r5,@.Lbbit_target
    bbit0_d_t_b_c_s9_end:
	nop_s

    ; bbit0.nt b,u6,s9
    .set bbit0_nt_b_u6_s9_target, @.Lbbit_target
    .set bbit0_nt_b_u6_s9_has_delay_slot,   0
    .set bbit0_nt_b_u6_s9_cc, 0
    bbit0_nt_b_u6_s9_start:
	bbit0.nt r4,u6_value,@.Lbbit_target
    bbit0_nt_b_u6_s9_end:

    ; bbit0.d.nt b,u6,s9
    .set bbit0_d_nt_b_u6_s9_target, @.Lbbit_target
    .set bbit0_d_nt_b_u6_s9_has_delay_slot,   1
    .set bbit0_d_nt_b_u6_s9_cc, 0
    bbit0_d_nt_b_u6_s9_start:
	bbit0.d.nt r4,u6_value,@.Lbbit_target
    bbit0_d_nt_b_u6_s9_end:
	nop_s

    ; bbit0.nt b,u6,s9
    .set bbit0_t_b_u6_s9_target, @.Lbbit_target
    .set bbit0_t_b_u6_s9_has_delay_slot,   0
    .set bbit0_t_b_u6_s9_cc, 0
    bbit0_t_b_u6_s9_start:
	bbit0.t r4,u6_value,@.Lbbit_target
    bbit0_t_b_u6_s9_end:

    ; bbit0.d.nt b,u6,s9
    .set bbit0_d_t_b_u6_s9_target, @.Lbbit_target
    .set bbit0_d_t_b_u6_s9_has_delay_slot,   1
    .set bbit0_d_t_b_u6_s9_cc, 0
    bbit0_d_t_b_u6_s9_start:
	bbit0.d.t r4,u6_value,@.Lbbit_target
    bbit0_d_t_b_u6_s9_end:
	nop_s

    ; bbit0.nt b,limm,s9
    .set bbit0_nt_b_limm_s9_target, @.Lbbit_target
    .set bbit0_nt_b_limm_s9_has_delay_slot,   0
    .set bbit0_nt_b_limm_s9_cc, 0
    bbit0_nt_b_limm_s9_start:
	bbit0.nt r4,limm_value,@.Lbbit_target
    bbit0_nt_b_limm_s9_end:

    ; bbit0.t b,limm,s9
    .set bbit0_t_b_limm_s9_target, @.Lbbit_target
    .set bbit0_t_b_limm_s9_has_delay_slot,   0
    .set bbit0_t_b_limm_s9_cc, 0
    bbit0_t_b_limm_s9_start:
	bbit0.t r4,limm_value,@.Lbbit_target
    bbit0_t_b_limm_s9_end:

    ; bbit0.nt limm,c,s9
    .set bbit0_nt_limm_c_s9_target, @.Lbbit_target
    .set bbit0_nt_limm_c_s9_has_delay_slot,   0
    .set bbit0_nt_limm_c_s9_cc, 0
    bbit0_nt_limm_c_s9_start:
	bbit0.nt limm_value,r4,@.Lbbit_target
    bbit0_nt_limm_c_s9_end:

    ; bbit0.t limm,c,s9
    .set bbit0_t_limm_c_s9_target, @.Lbbit_target
    .set bbit0_t_limm_c_s9_has_delay_slot,   0
    .set bbit0_t_limm_c_s9_cc, 0
    bbit0_t_limm_c_s9_start:
	bbit0.t limm_value,r4,@.Lbbit_target
    bbit0_t_limm_c_s9_end:

    ; bbit0.nt limm,u6,s9
    .set bbit0_nt_limm_u6_s9_target, @.Lbbit_target
    .set bbit0_nt_limm_u6_s9_has_delay_slot,   0
    .set bbit0_nt_limm_u6_s9_cc, 0
    bbit0_nt_limm_u6_s9_start:
	bbit0.nt limm_value,u6_value,@.Lbbit_target
    bbit0_nt_limm_u6_s9_end:

    ; bbit0.t limm,u6,s9
    .set bbit0_t_limm_u6_s9_target, @.Lbbit_target
    .set bbit0_t_limm_u6_s9_has_delay_slot,   0
    .set bbit0_t_limm_u6_s9_cc, 0
    bbit0_t_limm_u6_s9_start:
	bbit0.t limm_value,u6_value,@.Lbbit_target
    bbit0_t_limm_u6_s9_end:

    ; bbit1.nt b,c,s9
    .set bbit1_nt_b_c_s9_target, @.Lbbit_target
    .set bbit1_nt_b_c_s9_has_delay_slot,   0
    .set bbit1_nt_b_c_s9_cc, 0
    bbit1_nt_b_c_s9_start:
	bbit1.nt r4,r5,@.Lbbit_target
    bbit1_nt_b_c_s9_end:

    ; bbit1.d.nt b,c,s9
    .set bbit1_d_nt_b_c_s9_target, @.Lbbit_target
    .set bbit1_d_nt_b_c_s9_has_delay_slot,   1
    .set bbit1_d_nt_b_c_s9_cc, 0
    bbit1_d_nt_b_c_s9_start:
	bbit1.d.nt r4,r5,@.Lbbit_target
    bbit1_d_nt_b_c_s9_end:
	nop_s

    ; bbit1.t b,c,s9
    .set bbit1_t_b_c_s9_target, @.Lbbit_target
    .set bbit1_t_b_c_s9_has_delay_slot,   0
    .set bbit1_t_b_c_s9_cc, 0
    bbit1_t_b_c_s9_start:
	bbit1.t r4,r5,@.Lbbit_target
    bbit1_t_b_c_s9_end:

    ; bbit1.d.t b,c,s9
    .set bbit1_d_t_b_c_s9_target, @.Lbbit_target
    .set bbit1_d_t_b_c_s9_has_delay_slot,   1
    .set bbit1_d_t_b_c_s9_cc, 0
    bbit1_d_t_b_c_s9_start:
	bbit1.d.t r4,r5,@.Lbbit_target
    bbit1_d_t_b_c_s9_end:
	nop_s

    ; bbit1.nt b,u6,s9
    .set bbit1_nt_b_u6_s9_target, @.Lbbit_target
    .set bbit1_nt_b_u6_s9_has_delay_slot,   0
    .set bbit1_nt_b_u6_s9_cc, 0
    bbit1_nt_b_u6_s9_start:
	bbit1.nt r4,u6_value,@.Lbbit_target
    bbit1_nt_b_u6_s9_end:

    ; bbit1.d.nt b,u6,s9
    .set bbit1_d_nt_b_u6_s9_target, @.Lbbit_target
    .set bbit1_d_nt_b_u6_s9_has_delay_slot,   1
    .set bbit1_d_nt_b_u6_s9_cc, 0
    bbit1_d_nt_b_u6_s9_start:
	bbit1.d.nt r4,u6_value,@.Lbbit_target
    bbit1_d_nt_b_u6_s9_end:
	nop_s

    ; bbit1.nt b,u6,s9
    .set bbit1_t_b_u6_s9_target, @.Lbbit_target
    .set bbit1_t_b_u6_s9_has_delay_slot,   0
    .set bbit1_t_b_u6_s9_cc, 0
    bbit1_t_b_u6_s9_start:
	bbit1.t r4,u6_value,@.Lbbit_target
    bbit1_t_b_u6_s9_end:

    ; bbit1.d.nt b,u6,s9
    .set bbit1_d_t_b_u6_s9_target, @.Lbbit_target
    .set bbit1_d_t_b_u6_s9_has_delay_slot,   1
    .set bbit1_d_t_b_u6_s9_cc, 0
    bbit1_d_t_b_u6_s9_start:
	bbit1.d.t r4,u6_value,@.Lbbit_target
    bbit1_d_t_b_u6_s9_end:
	nop_s

    ; bbit1.nt b,limm,s9
    .set bbit1_nt_b_limm_s9_target, @.Lbbit_target
    .set bbit1_nt_b_limm_s9_has_delay_slot,   0
    .set bbit1_nt_b_limm_s9_cc, 0
    bbit1_nt_b_limm_s9_start:
	bbit1.nt r4,limm_value,@.Lbbit_target
    bbit1_nt_b_limm_s9_end:

    ; bbit1.t b,limm,s9
    .set bbit1_t_b_limm_s9_target, @.Lbbit_target
    .set bbit1_t_b_limm_s9_has_delay_slot,   0
    .set bbit1_t_b_limm_s9_cc, 0
    bbit1_t_b_limm_s9_start:
	bbit1.t r4,limm_value,@.Lbbit_target
    bbit1_t_b_limm_s9_end:

    ; bbit1.nt limm,c,s9
    .set bbit1_nt_limm_c_s9_target, @.Lbbit_target
    .set bbit1_nt_limm_c_s9_has_delay_slot,   0
    .set bbit1_nt_limm_c_s9_cc, 0
    bbit1_nt_limm_c_s9_start:
	bbit1.nt limm_value,r4,@.Lbbit_target
    bbit1_nt_limm_c_s9_end:

    ; bbit1.t limm,c,s9
    .set bbit1_t_limm_c_s9_target, @.Lbbit_target
    .set bbit1_t_limm_c_s9_has_delay_slot,   0
    .set bbit1_t_limm_c_s9_cc, 0
    bbit1_t_limm_c_s9_start:
	bbit1.t limm_value,r4,@.Lbbit_target
    bbit1_t_limm_c_s9_end:

    ; bbit1.nt limm,u6,s9
    .set bbit1_nt_limm_u6_s9_target, @.Lbbit_target
    .set bbit1_nt_limm_u6_s9_has_delay_slot,   0
    .set bbit1_nt_limm_u6_s9_cc, 0
    bbit1_nt_limm_u6_s9_start:
	bbit1.nt limm_value,u6_value,@.Lbbit_target
    bbit1_nt_limm_u6_s9_end:

    ; bbit1.t limm,u6,s9
    .set bbit1_t_limm_u6_s9_target, @.Lbbit_target
    .set bbit1_t_limm_u6_s9_has_delay_slot,   0
    .set bbit1_t_limm_u6_s9_cc, 0
    bbit1_t_limm_u6_s9_start:
	bbit1.t limm_value,u6_value,@.Lbbit_target
    bbit1_t_limm_u6_s9_end:
#endif /* TEST_BBIT */

#ifdef TEST_BCC
.Lbcc_target:
    ; bcc s21
    .set bcc_s21_target, @.Lbcc_target
    .set bcc_s21_has_delay_slot,   0
    .set bcc_s21_cc, 1
    bcc_s21_start:
	; beq @bcc_s21_target
	beq @.Lbcc_target
    bcc_s21_end:

    ; bcc.d s21
    .set bcc_d_s21_target, @.Lbcc_target
    .set bcc_d_s21_has_delay_slot,   1
    .set bcc_d_s21_cc, 1
    bcc_d_s21_start:
	beq.d @bcc_d_s21_target
    bcc_d_s21_end:
	nop_s

.Lbcc_s_target:
    ; beq_s s10
    .set beq_s_s10_target, @.Lbcc_s_target
    .set beq_s_s10_has_delay_slot,   0
    .set beq_s_s10_cc, 1
    beq_s_s10_start:
	# beq_s.d @beq_s_s10_target
	beq_s @.Lbcc_s_target
    beq_s_s10_end:

    ; bne_s s10
    .set bne_s_s10_target, @.Lbcc_s_target
    .set bne_s_s10_has_delay_slot,   0
    .set bne_s_s10_cc, 2
    bne_s_s10_start:
	bne_s @bne_s_s10_target
    bne_s_s10_end:

    ; bgt_s s7
    .set bgt_s_s7_target, @.Lbcc_s_target
    .set bgt_s_s7_has_delay_slot,   0
    .set bgt_s_s7_cc, 0x9
    bgt_s_s7_start:
	bgt_s @bgt_s_s7_target
    bgt_s_s7_end:

    ; bge_s s7
    .set bge_s_s7_target, @.Lbcc_s_target
    .set bge_s_s7_has_delay_slot,   0
    .set bge_s_s7_cc, 0xA
    bge_s_s7_start:
	bge_s @bge_s_s7_target
    bge_s_s7_end:

    ; blt_s s7
    .set blt_s_s7_target, @.Lbcc_s_target
    .set blt_s_s7_has_delay_slot,   0
    .set blt_s_s7_cc, 0xB
    blt_s_s7_start:
	blt_s @blt_s_s7_target
    blt_s_s7_end:

    ; ble_s s7
    .set ble_s_s7_target, @.Lbcc_s_target
    .set ble_s_s7_has_delay_slot,   0
    .set ble_s_s7_cc, 0xC
    ble_s_s7_start:
	ble_s @ble_s_s7_target
    ble_s_s7_end:

    ; bhi_s s7
    .set bhi_s_s7_target, @.Lbcc_s_target
    .set bhi_s_s7_has_delay_slot,   0
    .set bhi_s_s7_cc, 0xD
    bhi_s_s7_start:
	bhi_s @bhi_s_s7_target
    bhi_s_s7_end:

    ; bhs_s s7
    .set bhs_s_s7_target, @.Lbcc_s_target
    .set bhs_s_s7_has_delay_slot,   0
    .set bhs_s_s7_cc, 0x6
    bhs_s_s7_start:
	bhs_s @bhs_s_s7_target
    bhs_s_s7_end:

    ; blo_s s7
    .set blo_s_s7_target, @.Lbcc_s_target
    .set blo_s_s7_has_delay_slot,   0
    .set blo_s_s7_cc, 0x5
    blo_s_s7_start:
	blo_s @blo_s_s7_target
    blo_s_s7_end:

    ; bls_s s7
    .set bls_s_s7_target, @.Lbcc_s_target
    .set bls_s_s7_has_delay_slot,   0
    .set bls_s_s7_cc, 0xE
    bls_s_s7_start:
	bls_s @bls_s_s7_target
    bls_s_s7_end:
#endif /* TEST_BCC */

#ifdef TEST_BI
    ; bi [c]
    .set bi_c_target, @bi_c_end + (@r7_value << 2)
    .set bi_c_has_delay_slot,   0
    .set bi_c_cc, 0
    bi_c_start:
	bi [r7]
    bi_c_end:

    ; bih [c]
    .set bih_c_target, @bih_c_end + (@r7_value << 1)
    .set bih_c_has_delay_slot,   0
    .set bih_c_cc, 0
    bih_c_start:
	bih [r7]
    bih_c_end:
#endif /* TEST_BI */

#ifdef TEST_BL
.Lbl_target:
    ; bl s25
    .set bl_s25_target, @.Lbl_target
    .set bl_s25_has_delay_slot,   0
    .set bl_s25_cc, 0
    bl_s25_start:
	bl @bl_s25_target
    bl_s25_end:

    ; bl.d s25
    .set bl_d_s25_target, @.Lbl_target
    .set bl_d_s25_has_delay_slot,   1
    .set bl_d_s25_cc, 0
    bl_d_s25_start:
	bl.d @bl_d_s25_target
    bl_d_s25_end:
	nop_s

    ; bl_s s13
    .set bl_s_s13_target, @.Lbl_target
    .set bl_s_s13_has_delay_slot,   0
    .set bl_s_s13_cc, 0
    bl_s_s13_start:
	bl_s @bl_s_s13_target
    bl_s_s13_end:

    ; blcc s21
    .set blcc_s21_target, @.Lbl_target
    .set blcc_s21_has_delay_slot,   0
    .set blcc_s21_cc, 1
    blcc_s21_start:
	bleq @blcc_s21_target
    blcc_s21_end:

    ; blcc.d s21
    .set blcc_d_s21_target, @.Lbl_target
    .set blcc_d_s21_has_delay_slot,   1
    .set blcc_d_s21_cc, 2
    blcc_d_s21_start:
	blnz.d @blcc_d_s21_target
    blcc_d_s21_end:
	nop_s
#endif /* TEST_BL */

#ifdef TEST_BRCC
.Lbrcc_target:
    ; breq.nt b,c,s9
    .set breq_nt_b_c_s9_target, @.Lbrcc_target
    .set breq_nt_b_c_s9_has_delay_slot,   0
    .set breq_nt_b_c_s9_cc, 1
    breq_nt_b_c_s9_start:
	breq.nt r4,r5,@.Lbrcc_target
    breq_nt_b_c_s9_end:

    ; breq.d.nt b,c,s9
    .set breq_d_nt_b_c_s9_target, @.Lbrcc_target
    .set breq_d_nt_b_c_s9_has_delay_slot,   1
    .set breq_d_nt_b_c_s9_cc, 1
    breq_d_nt_b_c_s9_start:
	breq.d.nt r4,r5,@.Lbrcc_target
    breq_d_nt_b_c_s9_end:
	nop_s

    ; breq.t b,c,s9
    .set breq_t_b_c_s9_target, @.Lbrcc_target
    .set breq_t_b_c_s9_has_delay_slot,   0
    .set breq_t_b_c_s9_cc, 1
    breq_t_b_c_s9_start:
	breq.t r4,r5,@.Lbrcc_target
    breq_t_b_c_s9_end:

    ; breq.d.t b,c,s9
    .set breq_d_t_b_c_s9_target, @.Lbrcc_target
    .set breq_d_t_b_c_s9_has_delay_slot,   1
    .set breq_d_t_b_c_s9_cc, 1
    breq_d_t_b_c_s9_start:
	breq.d.t r4,r5,@.Lbrcc_target
    breq_d_t_b_c_s9_end:
	nop_s

    ; breq.nt b,u6,s9
    .set breq_nt_b_u6_s9_target, @.Lbrcc_target
    .set breq_nt_b_u6_s9_has_delay_slot,   0
    .set breq_nt_b_u6_s9_cc, 1
    breq_nt_b_u6_s9_start:
	breq.nt r4,u6_value,@.Lbrcc_target
    breq_nt_b_u6_s9_end:

    ; breq.d.nt b,u6,s9
    .set breq_d_nt_b_u6_s9_target, @.Lbrcc_target
    .set breq_d_nt_b_u6_s9_has_delay_slot,   1
    .set breq_d_nt_b_u6_s9_cc, 1
    breq_d_nt_b_u6_s9_start:
	breq.d.nt r4,u6_value,@.Lbrcc_target
    breq_d_nt_b_u6_s9_end:
	nop_s

    ; breq.nt b,u6,s9
    .set breq_t_b_u6_s9_target, @.Lbrcc_target
    .set breq_t_b_u6_s9_has_delay_slot,   0
    .set breq_t_b_u6_s9_cc, 1
    breq_t_b_u6_s9_start:
	breq.t r4,u6_value,@.Lbrcc_target
    breq_t_b_u6_s9_end:

    ; breq.d.nt b,u6,s9
    .set breq_d_t_b_u6_s9_target, @.Lbrcc_target
    .set breq_d_t_b_u6_s9_has_delay_slot,   1
    .set breq_d_t_b_u6_s9_cc, 1
    breq_d_t_b_u6_s9_start:
	breq.d.t r4,u6_value,@.Lbrcc_target
    breq_d_t_b_u6_s9_end:
	nop_s

    ; breq.nt b,limm,s9
    .set breq_nt_b_limm_s9_target, @.Lbrcc_target
    .set breq_nt_b_limm_s9_has_delay_slot,   0
    .set breq_nt_b_limm_s9_cc, 1
    breq_nt_b_limm_s9_start:
	breq.nt r4,limm_value,@.Lbrcc_target
    breq_nt_b_limm_s9_end:

    ; breq.t b,limm,s9
    .set breq_t_b_limm_s9_target, @.Lbrcc_target
    .set breq_t_b_limm_s9_has_delay_slot,   0
    .set breq_t_b_limm_s9_cc, 1
    breq_t_b_limm_s9_start:
	breq.t r4,limm_value,@.Lbrcc_target
    breq_t_b_limm_s9_end:

    ; breq.nt limm,c,s9
    .set breq_nt_limm_c_s9_target, @.Lbrcc_target
    .set breq_nt_limm_c_s9_has_delay_slot,   0
    .set breq_nt_limm_c_s9_cc, 1
    breq_nt_limm_c_s9_start:
	breq.nt limm_value,r4,@.Lbrcc_target
    breq_nt_limm_c_s9_end:

    ; breq.t limm,c,s9
    .set breq_t_limm_c_s9_target, @.Lbrcc_target
    .set breq_t_limm_c_s9_has_delay_slot,   0
    .set breq_t_limm_c_s9_cc, 1
    breq_t_limm_c_s9_start:
	breq.t limm_value,r4,@.Lbrcc_target
    breq_t_limm_c_s9_end:

    ; breq.nt limm,u6,s9
    .set breq_nt_limm_u6_s9_target, @.Lbrcc_target
    .set breq_nt_limm_u6_s9_has_delay_slot,   0
    .set breq_nt_limm_u6_s9_cc, 1
    breq_nt_limm_u6_s9_start:
	breq.nt limm_value,u6_value,@.Lbrcc_target
    breq_nt_limm_u6_s9_end:

    ; breq.t limm,u6,s9
    .set breq_t_limm_u6_s9_target, @.Lbrcc_target
    .set breq_t_limm_u6_s9_has_delay_slot,   0
    .set breq_t_limm_u6_s9_cc, 1
    breq_t_limm_u6_s9_start:
	breq.t limm_value,u6_value,@.Lbrcc_target
    breq_t_limm_u6_s9_end:

    ; brne_s b,0,s8
    .set brne_s_b_0_s8_target, @.Lbrcc_target
    .set brne_s_b_0_s8_has_delay_slot,   0
    .set brne_s_b_0_s8_cc, 1
    brne_s_b_0_s8_start:
	brne r12,0,@.Lbrcc_target
    brne_s_b_0_s8_end:

    ; breq_s b,0,s8
    .set breq_s_b_0_s8_target, @.Lbrcc_target
    .set breq_s_b_0_s8_has_delay_slot,   0
    .set breq_s_b_0_s8_cc, 1
    breq_s_b_0_s8_start:
	breq r12,0,@.Lbrcc_target
    breq_s_b_0_s8_end:
#endif /* TEST_BRCC */

#ifdef TEST_JLI
    ; jli_s u10
    .set jli_s_u10_target, @jli_target
    .set jli_s_u10_has_delay_slot,   0
    .set jli_s_u10_cc, 0
    jli_s_u10_start:
	jli_s jli_offset
    jli_s_u10_end:
#endif

#ifdef TEST_LEAVE_S
    ; leave_s
    .set leave_s_target, @blink_value
    .set leave_s_has_delay_slot,   0
    .set leave_s_cc, 0
    leave_s_start:
	; leave_s [r13-gp,fp,blink,pcl]
	leave_s (14 + 16 + 32 + 64)
    leave_s_end:
#endif

#ifdef TEST_LPCC
    ; lpcc
    .set lpcc_u7_target, @.Llpcc_end
    .set lpcc_u7_has_delay_slot,   0
    .set lpcc_u7_cc, 1
    lpcc_u7_start:
	lpeq @lpcc_u7_target
    lpcc_u7_end:
	nop
	nop
.Llpcc_end:
#endif

.Lend:

	.section	.note.GNU-stack,"",@progbits
