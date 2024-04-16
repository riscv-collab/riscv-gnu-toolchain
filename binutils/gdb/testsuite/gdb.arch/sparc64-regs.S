/* Copyright 2017-2024 Free Software Foundation, Inc.

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
   KAT for decoding various sparc64 registers. */

.section ".text"
.align 4

.global main
.type   main, #function
main:
	call test_ccr
	nop
	call test_fsr
	nop
	retl
	nop
.size main, .-main

.type test_ccr, #function
test_ccr:
	.cfi_startproc
	wr   %g0, 0x01, %ccr
	wr   %g0, 0x02, %ccr
	wr   %g0, 0x03, %ccr
	wr   %g0, 0x04, %ccr
	wr   %g0, 0x05, %ccr
	wr   %g0, 0x06, %ccr
	wr   %g0, 0x07, %ccr
	wr   %g0, 0x08, %ccr
	wr   %g0, 0x09, %ccr
	wr   %g0, 0x0a, %ccr
	wr   %g0, 0x0b, %ccr
	wr   %g0, 0x0c, %ccr
	wr   %g0, 0x0d, %ccr
	wr   %g0, 0x0e, %ccr
	wr   %g0, 0x0f, %ccr

	wr   %g0, 0x10, %ccr
	wr   %g0, 0x20, %ccr
	wr   %g0, 0x30, %ccr
	wr   %g0, 0x40, %ccr
	wr   %g0, 0x50, %ccr
	wr   %g0, 0x60, %ccr
	wr   %g0, 0x70, %ccr
	wr   %g0, 0x80, %ccr
	wr   %g0, 0x90, %ccr
	wr   %g0, 0xa0, %ccr
	wr   %g0, 0xb0, %ccr
	wr   %g0, 0xc0, %ccr
	wr   %g0, 0xd0, %ccr
	wr   %g0, 0xe0, %ccr
	wr   %g0, 0xf0, %ccr

	retl
	nop
	.cfi_endproc
.size test_ccr, .-test_ccr

.type test_fsr, #function
test_fsr:
	.cfi_startproc
	wr   %g0, 4, %fprs
	setx flags, %l1, %l0
	mov  1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1	! sparc64-regs.exp: after first %fsr
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 14, %l1	! move to fsr.tem fields
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1
	stx  %l1, [%l0]
	ldx  [%l0], %fsr
	sllx %l1, 1, %l1

	retl
	nop
	.cfi_endproc
.size test_fsr, .-test_fsr

.section ".data"
.align 8
flags: .xword 0x0000000000000000
.size flags, .-flags
	.section	.note.GNU-stack,"",@progbits
