/* Copyright 2019-2024 Free Software Foundation, Inc.

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

	.option nopic
	.text

	.align  1
	.globl  bar
	.type   bar, @function
bar:
	tail       1f
	.size   bar, .-func

	.align  1
	.globl  func
	.type   func, @function
func:
	/* A fake instruction of either 6 or 8 bytes.  This is never
	   executed, but the prologue scanner will try to decode it.
	   These long instructions are ISA extensions, use .byte rather
	   than an actual instruction mnemonic so that the test can be
	   compiled with a toolchain that doesn't include any long
	   instruction extensions.  */
#if BAD_INSN_LEN == 6
	/* A fake 6 byte instruction.  */
	.byte 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00
#elif BAD_INSN_LEN == 8
	/* A fake 8 byte instruction.  */
	.byte 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#else
#error unexpected bad instruction length
#endif

1:
	addi    sp,sp,-16
#if __SIZEOF_POINTER__ == 8
	sd      s0,8(sp)
#else
	sw      s0,8(sp)
#endif
	addi    s0,sp,16
	nop
#if __SIZEOF_POINTER__ == 8
	ld      s0,8(sp)
#else
	lw      s0,8(sp)
#endif
	addi    sp,sp,16
	jr      ra
	.size   func, .-func
	.section	.note.GNU-stack,"",@progbits
