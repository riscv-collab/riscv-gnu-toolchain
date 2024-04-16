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

.section .data
some_variable:
.long	0xdeadbeef

.section .text
.global	main
.type	main, @function

; Standard prologue.

.align 4
standard_prologue:
	push	blink
	sub	sp,sp,12
	st	r13, [sp, 0]
	st	r14, [sp, 4]
	st	r18, [sp, 8]
	add	r0, r1, r2
	ld	r18, [sp, 8]
	ld	r14, [sp, 4]
	ld	r13, [sp, 0]
	add	sp,sp,12
	pop	blink
	j	[blink]

; Standard prologue using short instructions.

.align 4
mini_prologue:
	push_s	blink
	sub_s	sp,sp,12
	; ST_S can store only some of the core registers.
	st_s	r13, [sp, 0]
	st_s	r15, [sp, 4]
	st_s	r14, [sp, 8]
	add	r0, r1, r2
	add	sp,sp,16
	j	[blink]

; Standard prologue without `sub sp,sp,INTEGER`.

.align 4
no_subsp_prologue:
	push	blink
	push	r13
	push	r20
	push	r25
	add	r0, r1, r2
	pop	r25
	pop	r20
	pop	r13
	pop	blink
	j	[blink]

; Standard prologue of leaf function.

.align 4
leaf_prologue:
	sub	sp,sp,8
	st	r13, [sp, 0]
	st	r15, [sp, 4]
	add	r0, r1, r2
	ld	r13, [sp, 0]
	ld	r15, [sp, 4]
	j.d	[blink]
	add	sp,sp,8

; Prologue with `push fp`.

.align 4
pushfp_prologue:
	push	r13
	push	r14
	push	fp
	; mov fp,sp is part of prologue, but this test will not verify that.
	; It will be checked later in the "arg_regs_fp" test.
	mov	fp, sp
	add	r0, r1, r2
	pop	fp
	pop	r14
	pop	r13
	j	[blink]

; Prologue with frame pointer and store relative to FP.

.align 4
fp_prologue_with_store:
	push	r13
	push	r14
	push	fp
	mov	fp, sp
	sub_s	sp,sp,4
	st	r15,[fp,-4]
	add	r0, r1, r2
	pop	r15
	pop	fp
	pop	r14
	pop	r13
	j	[blink]

; Verify that store of the non-callee saved registers is not part of prologue.
; Repeat this test for multiple registers, to check boundaries.  Also check
; with both ST and PUSH (aka ST.AW). We have to use multiple functions for
; this, because GDB would stop analisys at the first instruction that is not
; part of prologue.

.align 4
noncallee_saved_regs_r12_st:
	sub	sp,sp,8
	st	r13, [sp, 4]
	st	r12, [sp, 0]
	add	r0, r1, r2
	j.d	[blink]
	add	sp,sp,8

.align 4
noncallee_saved_regs_r12_push:
	push	r13
	push	r12
	add	r0, r1, r2
	j.d	[blink]
	add	sp,sp,8

.align 4
noncallee_saved_regs_r2_push:
	push	r13
	push	r2
	add	r0, r1, r2
	j.d	[blink]
	add	sp,sp,8

.align 4
noncallee_saved_regs_gp_push:
	push	r25
	push	gp
	add	r0, r1, r2
	j.d	[blink]
	add	sp,sp,8

; LP_COUNT is treated like a normal register.

.align 4
noncallee_saved_regs_lp_count:
	push	r25
	push	lp_count
	add	r0, r1, r2
	j.d	[blink]
	add	sp,sp,8

; BLINK is saved, but after an instruction that is not part of prologue.
; Currently arc_analyze_prologue stops analisys at the first intstruction
; that is not a part of prologue.  This might be not the best way, but it is
; what it is right now, so this test confirms this.

.align 4
noncallee_saved_regs_blink_out_of_prologue:
	push	r25
	push	gp
	push	blink
	add	r0, r1, r2
	j.d	[blink]
	add	sp,sp,12

; Saving arguments register via FP.

.align 4
arg_regs_fp:
	push	fp
	mov	fp, sp
	sub	sp, sp, 16
	st	r0, [fp, -4]
	st	r1, [fp, -8]
	st	r7, [fp, -12]
	st	r8, [fp, -16]
	add	r0, r1, r2
	add	sp,sp,16
	pop	fp
	j	[blink]

; Like the previous, but with mov_s.

.align 4
arg_regs_fp_mov_s:
	push	fp
	mov_s	fp, sp
	sub	sp, sp, 8
	st	r0, [fp, -4]
	; Not part of the prologue.
	st	r8, [fp, -8]
	add	r0, r1, r2
	add	sp,sp,8
	pop	fp
	j	[blink]

; Saving arguments register without FP.

.align 4
arg_regs_sp:
	sub	sp, sp, 24
	st	r0, [sp, 0]
	st	r1, [sp, 4]
	st	r7, [sp, 8]
	; Normally that would be done before saving args, but it is used as a
	; marker that saving arguments relatively to SP is considered part of
	; prologue.
	st	r13, [sp, 16]
	; Not part of the prologue.
	st	r8, [sp, 12]
	st	r14, [sp, 20]
	add	r0, r1, r2
	j.d	[blink]
	add	sp,sp,24

; ENTER_S that does nothing.

.align 4
enter_s_nop:
	; Effectively a nop.
	enter_s	0
	add	r0,r1,r2
	j	[blink]

; ENTER_S that stores BLINK.

.align 4
enter_s_blink:
	enter_s	32
	add	r0,r1,r2
	j.d	[blink]
	add	sp,sp,4

; ENTER_S that stores FP.

.align 4
enter_s_fp:
	enter_s	16
	add	r0,r1,r2
	j.d	[blink]
	add	sp,sp,4

; ENTER_S that stores R13, FP and BLINK.

.align 4
enter_s_r13:
	enter_s	(32 + 16 + 1)
	add	r0,r1,r2
	j.d	[blink]
	add	sp,sp,12

; ENTER_S that stores R13-R15

.align 4
enter_s_r15:
	enter_s	3
	add	r0,r1,r2
	j.d	[blink]
	add	sp,sp,12

; ENTER_S that stores everything it could.

.align 4
enter_s_all:
	enter_s	(32 + 16 + 14)
	add	r0,r1,r2
	j.d	[blink]
	add	sp,sp,64

; Deeper nesting.

.align 4
nested_prologue_inner:
	sub	sp,sp,8
	st	r18, [sp, 4]
	st	r13, [sp, 0]
	add	r0, r1, r2
	ld	r18, [sp, 4]
	ld	r13, [sp, 0]
	j.d	[blink]
	add	sp,sp,8

.align 4
nested_prologue_outer:
	push	blink
	sub	sp,sp,8
	st	r14, [sp, 0]
	st	r15, [sp, 4]
	bl	@nested_prologue_inner
	add	r0, r1, r2
	ld	r14, [sp, 0]
	ld	r15, [sp, 4]
	add	sp,sp,8
	pop	blink
	j	[blink]

; Prologue with maximum length.
; Expressions like (0xFFFFFFFF + 25) force assembler to use long immediate
; even for values that don't need it, thus letting us test maksimum prologue
; length without having huge frames.
.align 4
max_length_prologue:
	; Variadic args
	sub	sp,sp,(0xFFFFFFFF + 25) ; 24 bytes
	push	blink
	; Allocate space for 13 callee-saved and 8 arg regs.
	sub	sp,sp,(0xFFFFFFFF + 1 + 21 * 4)
	st	r13, [sp, 0]
	st	r14, [sp, 4]
	st	r15, [sp, 8]
	st	r16, [sp, 12]
	st	r17, [sp, 16]
	st	r18, [sp, 20]
	st	r19, [sp, 24]
	st	r20, [sp, 28]
	st	r21, [sp, 32]
	st	r22, [sp, 36]
	st	r23, [sp, 40]
	st	r24, [sp, 44]
	st	r25, [sp, 48]
	st	r0,  [sp, 52]
	st	r1,  [sp, 56]
	st	r2,  [sp, 60]
	st	r3,  [sp, 64]
	st	r4,  [sp, 68]
	st	r5,  [sp, 72]
	st	r6,  [sp, 76]
	st	r7,  [sp, 80]
	push	fp
	mov	fp,sp
	sub	sp,sp,(0xFFFFFFFF + 1 + 16) ; Space for local variables.
	; End of prologue.
	add	sp,sp,24 + 21 * 4 + 16
	j	[blink]

; Few tests that test that prologue analysis stops at branch.  There are four
; types of "branches": conditional and non-conditional, relative branches and
; absolute jumps.

.align 4
branch_in_prologue:
	push	r13
	b	@.L1
	; This store on stack is not a prologue.
	push	r14
.L1:
	add	r0,r1,r2
	j.d	[blink]
	add	sp,sp,4

.align 4
cond_branch_in_prologue:
	sub_s	sp,sp,8
	st_s	r13,[sp,4]
	; Doesn't matter if branch is taken or not.
	breq	r0,r1,@.L2
	; This store on stack is not a prologue.
	st_s	r14,[sp,0]
.L2:
	add	r0,r1,r2
	pop	fp
	j.d	[blink]
	add	sp,sp,8

.align 4
jump_in_prologue:
	push	r13
	j	@.L3
	; This store on stack is not a prologue.
	push	r14
.L3:
	add	r0,r1,r2
	j.d	[blink]
	add	sp,sp,4

.align 4
cond_jump_in_prologue:
	sub_s	sp,sp,8
	st_s	r13,[sp,4]
	; It doesn't matter if jump is taken or not - prologue analysis has to
	; stop before `jeq` in any case.
	jeq	@.L4
	; This store on stack is not a prologue.
	st_s	r14,[sp,0]
.L4:
	add	r0,r1,r2
	j.d	[blink]
	add	sp,sp,8

.align 4
predicated_insn:
	sub_s	sp,sp,12
	st_s	r13,[sp,8]
	st_s	r15,[sp,0]
	; Use SUB SP,SP,0 because it is otherwise a valid instruction for
	; prologue, so it will halt analysis purely because of its predicate.
	sub.eq	sp,sp,0 ; This is not a prologue anymore.
	st_s	r14,[sp,4]
	add	sp,sp,12
	j	[blink]

; Loops should halt prologue analysis.

.align 4
loop_in_prologue:
	push r25
	push lp_count
	mov lp_count, 4
	lp @.Lloop_end1
	push r26 ; Not part of prologue.
	add	r0, r1, r2
.Lloop_end1:
	add	r1, r1, r2
	pop r26
	add sp,sp,8
	pop r25
	j   [blink]

; Store of a constant value (not a register).

.align 4
store_constant:
	sub_s	sp,sp,12
	st_s	r13,[sp,8]
	st	0xdeadbeef,[sp,0]
	st_s	r14,[sp,4]
	add	sp,sp,12
	j	[blink]

; Test that store to immediate address halts prologue analysis.
.align 4
st_c_limm:
	push	r15
	st	r14,[@some_variable]
	push	r13
	add	sp,sp,8
	j	[blink]

; Store with AB writeback mode.

.align 4
st_ab_writeback:
	sub	sp,sp,8
	st	r13,[sp,4]
	st.ab	r14,[sp,-4]
	st	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store of a word with AS writeback mode.

.align 4
st_as_writeback:
	sub	sp,sp,12
	st	r13,[sp,8]
	st.as	r14,[sp,1] ; ST.AS, hence address is (offset << 2).
	st	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store of a halfword with AS writeback mode.

.align 4
sth_as_writeback:
	sub	sp,sp,12
	st	r13,[sp,8]
	sth.as	r14,[sp,2] ; STH.AS, hence address is (offset << 1).
	st	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store of a double word with AS writeback mode.  Shift is still 2, like ST!

.align 4
std_as_writeback:
	sub	sp,sp,16
	st	r13,[sp,12]
#ifdef __ARC_LL64__
	std.as	r14,[sp,1] ; STD.AS, hence address is (offset << 2).
#else
	st.as	r14,[sp,1] ; STD.AS, hence address is (offset << 2).
	st.as	r15,[sp,2] ; STD.AS, hence address is (offset << 2).
#endif
	st	r16,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store of the halfword.  R14 will not be reported as "saved".

.align 4
st_halfword:
	sub	sp,sp,12
	st	r13,[sp,8]
	sth	r14,[sp,4]
	st	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store of the halfword.  R14 will not be reported as "saved".

.align 4
sts_halfword:
	sub	sp,sp,12
	st	r13,[sp,8]
	mov	r13,sp
	sth_s	r14,[r13,4]
	st	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store of the byte.  R14 will not be reported as "saved".

.align 4
st_byte:
	sub	sp,sp,12
	st	r13,[sp,8]
	stb	r14,[sp,4]
	st	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store of the byte.  R14 will not be reported as "saved".

.align 4
sts_byte:
	sub	sp,sp,12
	st	r13,[sp,8]
	mov	r13,sp
	stb_s	r14,[r13,4]
	st	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store of the byte.  R14 will not be reported as "saved".

.align 4
sts_byte_sp:
	sub	sp,sp,12
	st	r13,[sp,8]
	stb_s	r14,[sp,4]
	st	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Double word store, optionally available for ARC HS.

.align 4
st_double:
	sub	sp,sp,8
#ifdef __ARC_LL64__
	std	r14,[sp,0]
	std.aw	r18,[sp,-8]
	std.aw	0xdeadbeef,[sp,-8]
#else
	st	r14,[sp,0]
	st	r15,[sp,4]
	st.aw	r19,[sp,-4]
	st.aw	r18,[sp,-4]
	sub	sp,sp,8
#endif
	add	sp,sp,24
	j	[blink]

; Store relative to some register with a known value.

.align 4
r_relative_store:
	sub_s	sp,sp,12
	st_s	r13,[sp,8]
	mov	r13,sp
	; Check for both mov and mov_s in one testcase.
	mov_s	r12,r13
	st	r15,[r12,0]
	st_s	r14,[sp,4]
	add	sp,sp,12
	j	[blink]

; Store relative to some register with a known value using sub.
; Like a previous test, but register is assigned via sub, instead of mov.

.align 4
r_relative_sub_store:
	; Following is a complicated way to construct frame like this:
	; sub_s	sp,sp,12
	; st_s	r13,[sp,8]
	; st_s	r14,[sp,4]
	; st_s	r15,[sp,0]
	sub_s	sp,sp,12
	st_s	r13,[sp,8]
	sub	r13,sp,4
	st	r14,[r13,8]
	st_s	r15,[sp,0]
	add	sp,sp,12
	j	[blink]

; Like r_relative_store, but using st_s c,[b,u7] which has different opcode.

.align 4
r_relative_store_st_s:
	sub_s	sp,sp,12
	st_s	r13,[sp,8]
	mov	r13,sp
	st_s	r15,[r13,4]
	st_s	r14,[sp,0]
	add	sp,sp,12
	j	[blink]

; Store relative to some register with a unknown value.

.align 4
r_relative_store_unknown:
	sub_s	sp,sp,12
	st_s	r13,[sp,8]
	st	r15,[gp,0] ; GP value is not relative to SP.
	st_s	r14,[sp,4]
	add	sp,sp,12
	j	[blink]

; Store relative to some register with a unknown value, using st_s r0,[gp,s11].

.align 4
st_s_r0gp:
	sub_s	sp,sp,12
	st_s	r13,[sp,8]
	st_s	r0,[gp,0] ; GP value is not relative to SP.
	st_s	r14,[sp,4]
	add	sp,sp,12
	j	[blink]

; Check prologue that uses `push_s RR` instructions. `push_s b` and `push_s
; blink` use slightly different subopcodes.

.align 4
push_s_prologue:
	push_s	r12
	push_s	r0
	push_s	r3
	push_s	r13
	push_s	r1
	push_s	r14
	push_s	r15
	push_s	r2
	push_s	blink ; Also tested in mini_prologue ().
	add	sp,sp,(4 * 9)
	j	[blink]

; Check for SUB_S c,b,u3 presence - it doesn't affect prologue.

.align 4
sub_s_cbu3:
	push_s	r13
	sub_s	r0,r1,3
	push_s	r14
	add	sp,sp,8
	j	[blink]

; Check for SUB_S b,b,c presence - it doesn't affect prologue.

.align 4
sub_s_bbc:
	push_s	r13
	sub_s	r0,r0,r1
	push_s	r0
	push_s	r1
	push_s	r14
	add	sp,sp,16
	j	[blink]

; Check for SUB_S b,b,u5.

.align 4
sub_s_bbu5:
	push_s	r13
	sub_s	r2,r2,14
	push_s	r2
	push_s	r14
	add	sp,sp,12
	j	[blink]

; Check for SUB 0,b,c, which is effectively a noop (but it can set status
; flags).  It shouldn't stop prologue analysis.

.align 4
sub_0bc:
	push_s	r13
	sub	0,r1,r2
	sub.f	0,r3,r4
	push_s	r14
	add	sp,sp,8
	j	[blink]

; Check for SUB a,limm,c.

.align 4
sub_alimmb:
	push_s	r13
	sub	r13,0xdeadbeef,r14
	push_s	r14
	add	sp,sp,8
	j	[blink]

; Check for sub_s.ne b,b,b.  Has a condition code, hence should halt prologue.

.align 4
sub_s_ne_bbb:
	push_s	r13
	sub_s.ne  r13,r13,r13
	push_s	r14
	add	sp,sp,8
	j	[blink]

; Check MOV that uses LIMM values.

.align 4
mov_limm:
	push_s	r13
	mov	r13,0xdeadbeef
	push_s	r14
	add	sp,sp,4
	pop_s	r13
	j	[blink]

; Check MOV 0,c.

.align 4
mov0c_limm:
	push_s	r13
	mov	0,r13
	push_s	r14
	add	sp,sp,4
	pop_s	r13
	j	[blink]

; Check that MOV_S h,s3 doesn't prevent prologue analysis.

.align 4
mov_s_hs3:
	push_s	r13
	mov_s	r5,1
	push_s	r14
	add	sp,sp,8
	j	[blink]

; Check that MOV_S b,u8 doesn't prevent prologue analysis.

.align 4
mov_s_bu8:
	push_s	r13
	mov_s	r12,250
	push_s	r14
	add	sp,sp,8
	j	[blink]

; Check that `mov_s.ne b,h` halts prologue analysis.

.align 4
mov_s_ne_bh:
	push_s	r13
	mov_s.ne r13,r5
	push_s	r14
	add	sp,sp,8
	j	[blink]

; Check that register R12 which original value is not stored will not pop-up in
; the "Saved registers" list.

.align 4
unstored_reg:
	sub_s	sp,sp,12
	st_s	r13,[sp,0]
	st_s	r14,[sp,4]
	mov	r12,0x42
	st_s	r12,[sp,8]
	add	sp,sp,12
	j	[blink]

; Two stores at the same adddress.  GDB should report only the R14.

.align 4
double_store:
	sub_s	sp,sp,4
	st_s	r13,[sp,0]
	st_s	r14,[sp,0]
	add	sp,sp,4
	j	[blink]

; Test for a case where callee has an alloca or anything else that might
; modify stack dynamically in the function body - after the prologue.
; This assumes that FP is set properly, so that GDB can use it - this holds
; true for frames generated by GCC.

.align 4
alloca_outer:
	sub	sp,sp,8
	st	blink,[sp,4]
	st	fp,[sp,0]
	mov	fp,sp
	add	r0,r1,r2 ; Not a prologue anymore.
	sub	sp,sp,8
	bl	@alloca_inner
	add	sp,sp,8
	ld	fp,[sp,0]
	ld	blink,[sp,4]
	j.d	[blink]
	add	sp,sp,8

.align 4
alloca_inner:
	push	r13
	push	r14
	add	sp,sp,8
	j	[blink]


.align 4
main:
	push	blink
	# Create small section for GP-relative accesses.
	push	gp
	sub	sp,sp,16
	add	gp,sp,8
	bl	@standard_prologue
	bl	@mini_prologue
	bl	@no_subsp_prologue
	bl	@leaf_prologue
	bl	@pushfp_prologue
	bl	@fp_prologue_with_store
	bl	@noncallee_saved_regs_r12_st
	bl	@noncallee_saved_regs_r12_push
	bl	@noncallee_saved_regs_r2_push
	bl	@noncallee_saved_regs_gp_push
	bl	@noncallee_saved_regs_lp_count
	bl	@noncallee_saved_regs_blink_out_of_prologue
	bl	@arg_regs_fp
	bl	@arg_regs_fp_mov_s
	bl	@arg_regs_sp
	bl	@enter_s_nop
	bl	@enter_s_blink
	bl	@enter_s_fp
	bl	@enter_s_r13
	bl	@enter_s_r15
	bl	@enter_s_all
	bl	@nested_prologue_outer
	bl	@max_length_prologue
	bl	@branch_in_prologue
	bl	@cond_branch_in_prologue
	bl	@jump_in_prologue
	bl	@cond_jump_in_prologue
	bl	@predicated_insn
	bl	@loop_in_prologue
	bl	@store_constant
	bl	@st_c_limm
	bl	@st_ab_writeback
	bl	@st_as_writeback
	bl	@sth_as_writeback
	bl	@std_as_writeback
	bl	@st_halfword
	bl	@sts_halfword
	bl	@st_byte
	bl	@sts_byte
	bl	@sts_byte_sp
	bl	@st_double
	bl	@r_relative_store
	bl	@r_relative_sub_store
	bl	@r_relative_store_st_s
	bl	@r_relative_store_unknown
	bl	@st_s_r0gp
	bl	@push_s_prologue
	bl	@sub_s_cbu3
	bl	@sub_s_bbc
	bl	@sub_s_bbu5
	bl	@sub_0bc
	bl	@sub_alimmb
	bl	@sub_s_ne_bbb
	bl	@mov_limm
	bl	@mov0c_limm
	bl	@mov_s_hs3
	bl	@mov_s_bu8
	bl	@mov_s_ne_bh
	bl	@unstored_reg
	bl	@double_store
	bl	@alloca_outer
	add	sp,sp,16
	pop	gp
	pop	blink
	j_s     [blink]

.align 4
	.section	.note.GNU-stack,"",@progbits
