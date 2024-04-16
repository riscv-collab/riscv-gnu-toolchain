/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <christian.himpel@intel.com>

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
	.globl loop
	.type  loop, @function

loop:
	movl $0x2, %eax /* bp.1 */
.L1:
	cmpl $0, %eax
	je .L2
	decl %eax
	jmp .L1
.L2:
	ret /* bp.2 */
	.section	.note.GNU-stack,"",@progbits
