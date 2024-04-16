/* Copyright 2020-2024 Free Software Foundation, Inc.

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

#include <asm/unistd.h>

/* Define these for each architecture:

   1) RETURN_ADDRESS_REGNO: The register number representing the return
      address in the DWARF CFI.  It can be easily be looked up using
      `readelf --debug-dump=frames-interp` on an existing binary of that
      architecture, where it says `ra=X`.

   2) exit_0: a sequence of instruction to execute the exit syscall with
      argument 0.  */

#if defined(__x86_64__)

# define RETURN_ADDRESS_REGNO 16

.macro exit_0
	mov $__NR_exit, %rax
	mov $0, %rdi
	syscall
.endm

#elif defined(__i386__)

# define RETURN_ADDRESS_REGNO 8

.macro exit_0
	mov $__NR_exit, %eax
	mov $0, %ebx
	int $0x80
.endm

#elif defined(__aarch64__)

# define RETURN_ADDRESS_REGNO 30

.macro exit_0
	mov x0, #0
	mov x8, #__NR_exit
	svc #0
.endm

#elif defined(__arm__)

# define RETURN_ADDRESS_REGNO 14

.macro exit_0
	ldr r7, =__NR_exit
	ldr r0, =0
	swi 0x0
.endm

#elif defined __powerpc64__

# define RETURN_ADDRESS_REGNO 65

.macro exit_0
	li 0, __NR_exit  /* r0 - contains system call number */
	li 3, 0          /* r3 - contains first argument for sys call */
	sc
.endm

#else
# error "Unsupported architecture"
#endif

/* The following assembly program mimics this pseudo C program, where
   everything has been inlined:

    1 void bar(void) {
    2   nop;
    3 }
    4
    5 void foo(void) {
    6   nop;
    7   bar();
    8   nop;
    9 }
   10
   11 void _start(void) {
   12   nop;
   13   foo();
   14   nop;
   15   exit(0);
   16 }
*/

#if defined __powerpc64__
#  if _CALL_ELF == 2
.abiversion 2   /* Tell gdb what ELF version to use. */
.global _start
_start:
#  else
.abiversion 1   /* Tell gdb what ELF version to use. */
.align 2
.global _start
.section ".opd", "aw"
.align 3
_start:
.quad ._start,.TOC.@tocbase,0
.previous
.type ._start,@function
._start:
#  endif
#else
.global _start
_start:
#endif
.cfi_startproc

/* State that the return address for this frame is undefined. */
.cfi_undefined RETURN_ADDRESS_REGNO

.global __cu_low_pc
__cu_low_pc:

.global __start_low_pc
__start_low_pc:
	/* Line 12 */
	nop

.global __foo_low_pc
__foo_low_pc:
	/* Line 6 */
	nop

.global __bar_low_pc
__bar_low_pc:
	/* Line 2 */
	nop

.global __bar_high_pc
__bar_high_pc:
	/* Line 8 */
	nop

.global __foo_high_pc
__foo_high_pc:
	/* Line 14 */
	nop

	/* Line 15 */
	exit_0

.cfi_endproc

.global __start_high_pc
__start_high_pc:

.global __cu_high_pc
__cu_high_pc:
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
