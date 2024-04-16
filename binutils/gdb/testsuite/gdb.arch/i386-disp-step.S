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

/***********************************************/

/* Test call/ret.  */

	nop
	.global test_call
test_call:
	call test_call_subr
	nop
	.global test_ret_end
test_ret_end:
	nop

/***********************************************/

/* Absolute jump with leading prefixes.
   These don't occur in normal code, but gdb should still DTRT.  */

	nop
	.global test_prefixed_abs_jump
test_prefixed_abs_jump:
	ds
	jmp *test_prefixed_abs_jump_addr
	.data
test_prefixed_abs_jump_addr:
	.4byte test_prefixed_abs_jump_target
	.text
test_prefixed_abs_jump_target:
	nop
	.global test_prefixed_abs_jump_end
test_prefixed_abs_jump_end:
	nop

/***********************************************/

/* Test syscall.  */

	mov $0x14,%eax /* getpid */
	.global test_syscall
test_syscall:
	int $0x80
	nop
	.global test_syscall_end
test_syscall_end:
	nop

/***********************************************/

/* Test syscall again, this time with a prefix.
   These don't occur in normal code, but gdb should still DTRT.  */

	mov $0x14,%eax /* getpid */
	.global test_prefixed_syscall
test_prefixed_syscall:
	repnz
	int $0x80
	nop
	.global test_prefixed_syscall_end
test_prefixed_syscall_end:
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

/* all done */

	pushl $0
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
	.section	.note.GNU-stack,"",@progbits
