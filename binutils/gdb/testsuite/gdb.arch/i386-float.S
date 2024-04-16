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

   This file is part of the gdb testsuite.  */

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

#ifdef SYMBOL_PREFIX
# define SYMBOL(str)     CONCAT1(SYMBOL_PREFIX, str)
#else
# define SYMBOL(str)     str
#endif

	.text
	.globl SYMBOL(main)
SYMBOL(main):
	fninit
	fldt    val
	fldt    bigval
	fldt    smallval
	ret
.data
	val: .byte 0x00,0x00,0x45,0x07,0x11,0x19,0x22,0xe9,0xfe,0xbf
	smallval: .byte 0xe9,0xd0,0xd7,0x65,0x81,0xb7,0x43,0xc6,0x5c,0x6a
	bigval:   .byte 0x6a,0x45,0x90,0xdc,0x03,0xc1,0xf8,0xc6,0xe5,0x03
	.section	.note.GNU-stack,"",@progbits
