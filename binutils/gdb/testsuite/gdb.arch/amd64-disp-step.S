/* Copyright 2009-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   This file is part of the gdb testsuite.
   It tests displaced stepping over various insns that require special
   handling.  */

	.text

	.global main
main:
	nop

	callq	setup_signal_handler

	nop

/***********************************************/

/* test call/ret */

	.global test_call
test_call:
	call test_call_subr
	nop
	.global test_ret_end
test_ret_end:
	nop

/***********************************************/

/* test abs-jmp/rep-ret */

test_abs_jmp_setup:
	mov $test_abs_jmp_return,%rdx
	push %rdx
	mov $test_abs_jmp_subr,%rdx
	.global test_abs_jmp
test_abs_jmp:
	jmp *%rdx
test_abs_jmp_return:
	nop
	.global test_rep_ret_end
test_rep_ret_end:
	nop

/***********************************************/

/* test syscall */

	.global test_syscall
	mov $0x27,%eax /* getpid */
test_syscall:
	syscall
	nop
test_syscall_end:
	nop

/***********************************************/

/* Test stepping over int3.
   The prefixes are pointless, but it's possible, so we exercise it.  */

	nop
	.global test_int3
test_int3:
	repz
	repz
	int3
	nop
	.global test_int3_end
test_int3_end:
	nop

/***********************************************/

/* test rip-relative
   GDB picks a spare register to hold the rip-relative address.
   Exercise all the possibilities (rax-rdi, sans rsp).  */

	.global test_rip_rax
test_rip_rax:
	add answer(%rip),%rax
	.global test_rip_rax_end
test_rip_rax_end:
	nop

	.global test_rip_rbx
test_rip_rbx:
	add answer(%rip),%rbx
	.global test_rip_rbx_end
test_rip_rbx_end:
	nop

	.global test_rip_rcx
test_rip_rcx:
	add answer(%rip),%rcx
	.global test_rip_rcx_end
test_rip_rcx_end:
	nop

	.global test_rip_rdx
test_rip_rdx:
	add answer(%rip),%rdx
	.global test_rip_rdx_end
test_rip_rdx_end:
	nop

	.global test_rip_rbp
test_rip_rbp:
	add answer(%rip),%rbp
	.global test_rip_rbp_end
test_rip_rbp_end:
	nop

	.global test_rip_rsi
test_rip_rsi:
	add answer(%rip),%rsi
	.global test_rip_rsi_end
test_rip_rsi_end:
	nop

	.global test_rip_rdi
test_rip_rdi:
	add answer(%rip),%rdi
	.global test_rip_rdi_end
test_rip_rdi_end:
	nop

	.global test_jmp
test_jmp:
	jmpq 	*jmp_dest(%rip)
	nop
	.global test_jmp_end
test_jmp_end:
	nop

	/* skip over test data */
	jmp done

/* test rip-relative data */

answer:	.8byte 42

jmp_dest:
	.8byte	test_jmp_end

/***********************************************/

/* all done */

done:
	mov $0,%rdi
	call exit
	hlt

/***********************************************/

/* subroutine to help test call/ret */

test_call_subr:
	nop
	.global test_call_end
test_call_end:
	nop

	.global test_ret
test_ret:
	ret

/***********************************************/

/* subroutine to help test abs-jmp/rep-ret */

test_abs_jmp_subr:
	nop
	.global test_abs_jmp_end
test_abs_jmp_end:
	nop

	.global test_rep_ret
test_rep_ret:
	repz
	ret
	.section	.note.GNU-stack,"",@progbits
