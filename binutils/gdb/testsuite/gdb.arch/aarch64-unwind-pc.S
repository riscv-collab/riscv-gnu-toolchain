/* Copyright 2022-2024 Free Software Foundation, Inc.

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

        .text
        .globl main
        .type main,#function
main:
        .cfi_startproc
        stp     x29, x30, [sp, -16]!
        .cfi_def_cfa sp, 16
        .cfi_offset x29, 0
        .cfi_offset x30, 8
        bl test_func
        ldp     x29, x30, [sp], 16
        .cfi_restore x29
        .cfi_restore x30
        .cfi_def_cfa sp, 0
        mov     x0, 0
        ret
        .cfi_endproc

        .globl test_func
test_func:
        .cfi_startproc
        // Unwind x30 to a different value
        // CFA_val_expression x30 const2u 0x1234
        .cfi_escape 0x16, 30, 0x03, 0x0a, 0x34, 0x12
        // CFA_val_expression pc breg30 0
        .cfi_escape 0x16, 32, 0x02, 0x8e, 0x00
        mov     x0, x30
        .cfi_register 32, x0
        mov     x30, 0x1234
        ret     x0
        .cfi_endproc
	.section	.note.GNU-stack,"",@progbits
