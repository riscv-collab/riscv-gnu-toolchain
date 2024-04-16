/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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


/* This file implements simple Linux syscall wrappers, to be used by tests that
   need to know exactly where the syscall instructions are.  */

#include <asm/unistd.h>

/* The SYSCALL macro below current supports calling syscalls with up
   to 3 arguments, and, assumes the syscall never returns, like exec
   and exit.  If you need to call syscalls with more arguments or you
   need to call syscalls that actually return, you'll need to update
   the macros.  We don't bother with optimizing setting up fewer
   arguments for syscalls that take fewer arguments, as we're not
   optimizating for speed or space, but for maintainability.  */

#if defined(__x86_64__)

#define SYSCALL(NAME, NR)	\
.global NAME			;\
NAME:				;\
	mov $NR, %rax		;\
	/* rdi, rsi and rdx already contain the right arguments.  */ \
NAME ## _syscall:		;\
	syscall			;\
	ret			;

#elif defined(__i386__)

#define SYSCALL(NAME, NR)	\
.global NAME			;\
NAME:				;\
	mov $NR, %eax		;\
	mov 4(%esp), %ebx	;\
	mov 8(%esp), %ecx	;\
	mov 12(%esp), %edx	;\
NAME ## _syscall:		;\
	int $0x80		;\
	ret

#elif defined(__aarch64__)

#define SYSCALL(NAME, NR)	\
.global NAME			;\
NAME:				;\
	mov x8, NR		;\
	/* x0, x1 and x2 already contain the right arguments.  */ \
NAME ## _syscall:		;\
	svc #0

#else
# error "Unsupported architecture"
#endif

SYSCALL (my_execve, __NR_execve)

/* void my_exit (int code);  */

SYSCALL (my_exit, __NR_exit)

	.section	.note.GNU-stack,"",@progbits
