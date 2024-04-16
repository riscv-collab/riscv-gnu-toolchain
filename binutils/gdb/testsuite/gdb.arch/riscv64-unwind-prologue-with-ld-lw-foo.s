/* Copyright 2021-2024 Free Software Foundation, Inc.

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

/* This testcase contains a function where the 'ld', 'c.ld', 'lw' or 'c.lw'
   instruction is used in the prologue before the RA register have been saved
   on the stack.

   This mimics a pattern observed in the __pthread_clockjoin_ex function
   in libpthread.so.0 (from glibc-2.33-0ubuntu5) where a canary value is
   loaded and placed on the stack in order to detect stack smashing.

   The skeleton for this file was generated using the following command:

      gcc -x c -S -c -o - - <<EOT
        static long int __canary = 42;
        extern int bar ();
        int foo () { return bar(); }
      EOT

   The result of this command is modified in the following way:
     - The prologue is adapted to reserve 16 more bytes on the stack.
     - A part that simulates the installation of a canary on the stack is
       added.  The canary is loaded multiple times to simulate the use of
       various instructions that could do the work (ld or c.ld for a 64 bit
       canary, lw or c.lw for a 32 bit canary).
     - The epilogue is adjusted to be able to return properly.  The epilogue
       does not check the canary value since this testcase is only interested
       in ensuring GDB can scan the prologue.  */

	.option pic
	.text
	.data
	.align	3
	.type	__canary, @object
	.size	__canary, 8
__canary:
	.dword	42
	.text
	.align	1
	.globl	foo
	.type	foo, @function
foo:
	addi	sp,sp,-32
	lla	a5,__canary  # Load the fake canary address.
	lw	t4,0(a5)     # Load a 32 bit canary (use t4 to force the use of
			     # the non compressed instruction).
	ld	t4,0(a5)     # Load a 64 bit canary (use t4 to force the use of
			     # the non compressed instruction).
	c.lw 	a4,0(a5)     # Load a 32 bit canary using the compressed insn.
	c.ld 	a4,0(a5)     # Load a 64 bit canary using the compressed insn.
	sd	a4,0(sp)     # Place the fake canary on the stack.
	sd	ra,16(sp)
	sd	s0,8(sp)
	addi	s0,sp,32
	call	bar@plt
	mv	a5,a0
	mv	a0,a5
	ld	ra,16(sp)
	ld	s0,8(sp)
	addi	sp,sp,32
	jr	ra
	.size	foo, .-foo
	.section	.note.GNU-stack,"",@progbits
