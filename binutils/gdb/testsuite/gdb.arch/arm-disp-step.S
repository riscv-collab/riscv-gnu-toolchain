/* Copyright 2010-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

	.syntax unified
	.text
	.type main,%function
#if defined (__thumb__)
	.code   16
	.thumb_func
#endif
	.globl main
main:
	push {r7, lr}
	add     sp, sp, #4
	mov	r7, sp

	
	/* Test call and return */	
	.global test_call
test_call:
	bl test_call_subr
        .global test_ret_end
test_ret_end:
	nop			@ Location test_ret_end
	
	/* Test branch */
	bl test_branch

	/* Test ldr from pc */
	bl test_ldr_pc

	/* Test ldm/stm only in ARM mode */
#if !defined (__thumb__)
	bl test_ldm_stm_pc
#endif

	/* Test ldrX literal in ARM and Thumb-2 */
#if !defined (__thumb__) || defined(__thumb2__)
	bl test_ldr_literal
#endif

	/* Test ldr literal in Thumb */
#if defined(__thumb__)
	bl test_ldr_literal_16
#endif

	/* Test cbnz/cbz in Thumb-2 */
#if defined(__thumb2__)
	bl test_cbz_cbnz
#endif

	/* Test adr in Thumb and Thumb-2 */
#if defined(__thumb__)
	bl test_adr
#endif
	/* Test 32-bit adr in ARM and Thumb-2 */
#if defined(__thumb2__) || !defined(__thumb__)
	bl test_adr_32bit
#endif

	bl test_pop_pc
	
	/* Test str in ARM mode and Thumb-2 */
#if !defined(__thumb__)
	bl test_str_pc
#endif

	/* Test add with pc in Thumb and Thumb-2 */
#if defined(__thumb__)
	bl test_add_rn_pc
#endif
	/* Return */
	mov     sp, r7
	sub     sp, sp, #4
	movs 	r0, #0
	pop	{r7, pc}
	.size main, .-main
	
	.global test_call_subr
#if defined (__thumb__)
	.code   16
	.thumb_func
#endif
	.type test_call_subr, %function
test_call_subr:
	nop
        .global test_call_end
test_call_end:
        nop			@ Location test_call_end
        .global test_ret
test_ret:
        bx lr
	.size test_call_subr, .-test_call_subr

	
	.global test_branch
#if defined (__thumb__)
	.code   16
	.thumb_func
#endif
	.type test_branch, %function
test_branch:
	b	L_branch
	.global L_branch
L_branch:
	bx lr
	.size test_branch, .-test_branch

	.global test_ldr_pc
#if defined (__thumb__)
	.code   16
	.thumb_func
#endif
	.type test_ldr_pc, %function
test_ldr_pc:
	ldr	r1, [pc, #0]

	.global test_ldr_pc_ret
test_ldr_pc_ret:
	bx lr
	.size test_ldr_pc, .-test_ldr_pc

#if !defined (__thumb__)
	.global test_ldm_stm_pc
	.type test_ldm_stm_pc, %function
test_ldm_stm_pc:
	stmdb	sp!, {lr, pc}
	ldmia	sp!, {r0, r1}
	ldr	r0, .L1
	stmdb	sp!, {r0}
	.global test_ldm_pc
test_ldm_pc:
	ldmia	sp!, {pc}
	.global test_ldm_stm_pc_ret
test_ldm_stm_pc_ret:
	bx lr
	.align	2
.L1:
	.word	test_ldm_stm_pc_ret
	.size test_ldm_stm_pc, .-test_ldm_stm_pc
#endif
	
#if !defined (__thumb__) || defined(__thumb2__)
	.global test_ldr_literal
	.type test_ldr_literal, %function
test_ldr_literal:
	ldrh    r0, [pc]
	.global test_ldrsb_literal
test_ldrsb_literal:
	ldrsb   r0, [pc]
	.global test_ldrsh_literal
test_ldrsh_literal:
	ldrsh   r0, [pc]
	.global test_ldr_literal_end
test_ldr_literal_end:
	bx lr
	.size test_ldr_literal, .-test_ldr_literal
#endif

#if defined(__thumb__)
	.global test_ldr_literal_16
	.code   16
	.thumb_func
test_ldr_literal_16:
	ldr	r0, .L2
	.global test_ldr_literal_16_end
test_ldr_literal_16_end:
	bx lr
	.align	2
.L2:
	.word	test_ldr_literal_16
	.size test_ldr_literal_16, .-test_ldr_literal_16
#endif

#if defined(__thumb2__)
	.global test_cbz_cbnz
	.code   16
	.thumb_func
test_cbz_cbnz:
	movs 	r0, #0
	.global test_zero_cbnz
test_zero_cbnz:
	cbnz	r0, .L3
	.global test_zero_cbz
test_zero_cbz:
	cbz	r0, .L3
.L3:
	movs	r0, #1
	.global test_non_zero_cbz
test_non_zero_cbz:
	cbz	r0, .L4
	.global test_non_zero_cbnz
test_non_zero_cbnz:
	cbnz	r0, .L4
	nop
.L4:
	.global test_cbz_cbnz_end
test_cbz_cbnz_end:
	bx lr
	.size test_cbz_cbnz, .-test_cbz_cbnz
#endif

#if defined(__thumb__)
	.global test_adr
	.code   16
	.thumb_func
test_adr:
	adr	r0, .L8
	nop
	nop
	nop
.L8:
	.global test_adr_end
test_adr_end:
	bx lr
	.size test_adr, .-test_adr
#endif

#if defined(__thumb2__) || !defined(__thumb__)
	.global test_adr_32bit
#if defined(__thumb2__)
	.code   16
	.thumb_func
#endif
test_adr_32bit:
	adr     r0, .L6
	nop
.L6:
	nop
	.global test_adr_32bit_after
test_adr_32bit_after:
	adr     r0, .L6

	.global test_adr_32bit_end
test_adr_32bit_end:
	bx lr
	.size test_adr_32bit, .-test_adr_32bit
#endif

	.global test_pop_pc
	.type test_pop_pc, %function
#if defined(__thumb__)
	.code   16
	.thumb_func
#endif

test_pop_pc:
	ldr     r1, .L1_right
	ldr	r2, .L1_wrong
#if defined(__thumb__)
	movs    r0, #1
	orrs    r1, r0
	orrs	r2, r0
#endif
	push    {r1}
	push    {r2}
	.global test_pop_pc_1
test_pop_pc_1:
	pop     {r1, pc}

test_pop_pc_2_start:
	ldr r1, .L2_right
#if defined(__thumb__)
	movs    r0, #1
	orrs    r1, r0
#endif
	push	{r1}
	.global test_pop_pc_2
test_pop_pc_2:
	pop	{pc}

	/* Test pop instruction with full register list.  */
test_pop_pc_3_start:
	ldr     r1, .L3_right
	ldr	r2, .L3_wrong
#if defined(__thumb__)
	movs    r0, #1
	orrs    r1, r0
	orrs	r2, r0
#endif
	push 	{r7}
	push    {r1} /* Push the right address so that PC will get it.  */
	/* Push the wrong address so r0-r7 will get the wrong a	ddress.  If PC
	is set from any of them, we can get a FAIL.  */
	push	{r2} 
	push	{r2}
	push	{r2}
	push	{r2}
	push	{r2}
	push	{r2}
	push	{r2}
	push	{r2}
test_pop_pc_3:
	pop	{r0,r1,r2,r3,r4,r5,r6,r7,pc}
	.global test_pop_pc_ret
test_pop_pc_ret:
	pop	{r7}
	bx lr

	.global test_pop_pc_1_right
test_pop_pc_1_right:
	b	test_pop_pc_2_start /* right */
	.global test_pop_pc_1_wrong
test_pop_pc_1_wrong:
	b	test_pop_pc_2_start /* wrong */
	.global test_pop_pc_2_right
test_pop_pc_2_right:
	b	test_pop_pc_3_start /* right */
	.global test_pop_pc_2_wrong
test_pop_pc_2_wrong:
	b	test_pop_pc_3_start /* wrong */
	.global test_pop_pc_3_right
test_pop_pc_3_right:
	b	test_pop_pc_ret /* right */
	.global test_pop_pc_3_wrong
test_pop_pc_3_wrong:
	b	test_pop_pc_ret /* wrong */
	
	.align  2
.L1_right:
	.word   test_pop_pc_1_right
.L1_wrong:
	.word	test_pop_pc_1_wrong
.L2_right:
	.word   test_pop_pc_2_right
.L2_wrong:
	.word	test_pop_pc_2_wrong
.L3_right:
	.word   test_pop_pc_3_right
.L3_wrong:
	.word	test_pop_pc_3_wrong
	.size test_pop_pc, .-test_pop_pc

#if !defined(__thumb__)
#if defined (__thumb2__)
	.code   16
	.thumb_func
#endif
	.global test_str_pc
	.type test_str_pc, %function
test_str_pc:
	str     pc, [sp, #-4]
	ldr	r0, [sp, #-4]
	sub	r0, r0, pc
	/* compute offset again without displaced stepping.  */
	str     pc, [sp, #-4]
	ldr	r1, [sp, #-4]
	sub	r1, r1, pc

	/* r0 should be equal to r1.  */
	cmp	r0, r1
	bne	pc_offset_wrong

	.global pc_offset_right
pc_offset_right:
	b	test_str_pc_end

	.global pc_offset_wrong
pc_offset_wrong:
	nop

	.global test_str_pc_end
test_str_pc_end:
	bx lr
#endif

#if defined(__thumb__)
	.global test_add_rn_pc
	.code   16
	.thumb_func
test_add_rn_pc:
	mov	r3, 4
test_add_rn_pc_start:
	add	r3, pc
	.global test_add_rn_pc_end
test_add_rn_pc_end:
	bx lr
	.size test_add_rn_pc, .-test_add_rn_pc
#endif
	.section	.note.GNU-stack,"",%progbits
